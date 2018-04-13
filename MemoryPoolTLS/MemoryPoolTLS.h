#ifndef __MEMORYPOOLTLS__H__
#define __MEMORYPOOLTLS__H__

template <class DATA>
class CMemoryPoolTLS
{
private :
	/*-----------------------------------------------------------------*/
	// Data Block의 집합체(덩어리)
	/*-----------------------------------------------------------------*/
	class CChunk
	{
	public :
		enum e_CONFIG
		{
			eMAX_DATA_BLOCK = 100,
			eDATA_CODE		= 0x7fffffff
		};

		
		/*-------------------------------------------------------------*/
		// Data block
		/*-------------------------------------------------------------*/
		typedef struct st_DATA_BLOCK
		{
			unsigned __int64	uiDataCode;

			CChunk				*pChunk;
			DATA				data;
		} DATA_BLOCK;


	public :
		CChunk()
		{
			_lAllocCount = eMAX_DATA_BLOCK;
			_lFreeCount = eMAX_DATA_BLOCK;
		}

		virtual ~CChunk(void)
		{
		}

	public :
		/////////////////////////////////////////////////////////////////
		// Data Block 할당(Chunk 안에서)
		/////////////////////////////////////////////////////////////////
		DATA *Alloc(void)
		{
			_lAllocCount--;

			stDataBlock[_lAllocCount].uiDataCode = eDATA_CODE;
			stDataBlock[_lAllocCount].pChunk = this;

			if (_bPlacementNew)
				new (&stDataBlock[_lAllocCount].data) DATA;

			if (0 == _lAllocCount)
				_pMemoryPoolTLS->ChunkAlloc();

			return &stDataBlock[_lAllocCount].data;
		}

		bool Free()
		{
			InterlockedDecrement(&_lFreeCount);

			if (0 == _lFreeCount)
				_pMemoryPoolTLS->pChunkBlock->Free(this);

			return true;
		}

	public :
		/////////////////////////////////////////////////////////////////
		// Chunk가 들고있는 데이터 블록들
		/////////////////////////////////////////////////////////////////
		DATA_BLOCK stDataBlock[eMAX_DATA_BLOCK];

		/////////////////////////////////////////////////////////////////
		// 할당 된 갯수, 반환 된 갯수
		/////////////////////////////////////////////////////////////////
		long _lAllocCount;
		long _lFreeCount;

		/////////////////////////////////////////////////////////////////
		// Placement New 여부
		/////////////////////////////////////////////////////////////////
		bool _bPlacementNew;

		/////////////////////////////////////////////////////////////////
		// TLS 포인터(chunk 반환 시 필요)
		/////////////////////////////////////////////////////////////////
		CMemoryPoolTLS<DATA> *_pMemoryPoolTLS;
	};

public :
	CMemoryPoolTLS(bool bPlacementNew = true)
	{
		_dwTlsIndex = TlsAlloc();
		if (TLS_OUT_OF_INDEXES == _dwTlsIndex)
			_exit(-1);

		pChunkBlock = new CMemoryPool<CChunk>();

		_bPlacementNew = bPlacementNew;

		_lUseSize = 0;
		_iChunkSize = 0;
	}

	virtual ~CMemoryPoolTLS()
	{
		TlsFree(_dwTlsIndex);

		delete pChunkBlock;
	}


public :
	DATA *Alloc()
	{
		CChunk *pChunk = (CChunk *)TlsGetValue(_dwTlsIndex);
		if (nullptr == pChunk)
			pChunk = ChunkAlloc();

		return pChunk->Alloc();
	}
	
	bool Free(DATA *pData)
	{
		// 데이터 블록 포인터 체크
		CChunk::DATA_BLOCK *pBlock = (CChunk::DATA_BLOCK *)(((unsigned char *)pData)
			- sizeof(CChunk *) - sizeof(unsigned __int64));

		if (CChunk::eDATA_CODE != pBlock->uiDataCode)
			return false;

		return pBlock->pChunk->Free();
	}




private :
	CChunk *ChunkAlloc()
	{
		CChunk *pChunk = pChunkBlock->Alloc();
		if (!TlsSetValue(_dwTlsIndex, pChunk))
			return nullptr;

		new (pChunk)CChunk;

		pChunk->_bPlacementNew = _bPlacementNew;
		pChunk->_pMemoryPoolTLS = this;
		InterlockedIncrement(&_lUseSize);

		return pChunk;
	}



private :
	///////////////////////////////////////////////////////////////////////
	// Chunk를 취급하는 MemoryPool
	///////////////////////////////////////////////////////////////////////
	CMemoryPool<CChunk> *pChunkBlock;

	///////////////////////////////////////////////////////////////////////
	// TLS 플래그 배열 인덱스
	///////////////////////////////////////////////////////////////////////
	DWORD				_dwTlsIndex;

	int					_iChunkSize; // Chunk한 데이터 갯수(?)

	///////////////////////////////////////////////////////////////////////
	// 현재 사용량
	///////////////////////////////////////////////////////////////////////
	long				_lUseSize;

	///////////////////////////////////////////////////////////////////////
	// Placement New 여부
	///////////////////////////////////////////////////////////////////////
	bool				_bPlacementNew;
	
};

#endif