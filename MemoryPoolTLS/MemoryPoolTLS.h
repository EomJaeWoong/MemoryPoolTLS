#ifndef __MEMORYPOOLTLS__H__
#define __MEMORYPOOLTLS__H__

template <class DATA>
class CMemoryPoolTLS
{
private :
	/*-----------------------------------------------------------------*/
	// Data Block�� ����ü(���)
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
		// Data Block �Ҵ�(Chunk �ȿ���)
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
		// Chunk�� ����ִ� ������ ��ϵ�
		/////////////////////////////////////////////////////////////////
		DATA_BLOCK stDataBlock[eMAX_DATA_BLOCK];

		/////////////////////////////////////////////////////////////////
		// �Ҵ� �� ����, ��ȯ �� ����
		/////////////////////////////////////////////////////////////////
		long _lAllocCount;
		long _lFreeCount;

		/////////////////////////////////////////////////////////////////
		// Placement New ����
		/////////////////////////////////////////////////////////////////
		bool _bPlacementNew;

		/////////////////////////////////////////////////////////////////
		// TLS ������(chunk ��ȯ �� �ʿ�)
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
		// ������ ��� ������ üũ
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
	// Chunk�� ����ϴ� MemoryPool
	///////////////////////////////////////////////////////////////////////
	CMemoryPool<CChunk> *pChunkBlock;

	///////////////////////////////////////////////////////////////////////
	// TLS �÷��� �迭 �ε���
	///////////////////////////////////////////////////////////////////////
	DWORD				_dwTlsIndex;

	int					_iChunkSize; // Chunk�� ������ ����(?)

	///////////////////////////////////////////////////////////////////////
	// ���� ��뷮
	///////////////////////////////////////////////////////////////////////
	long				_lUseSize;

	///////////////////////////////////////////////////////////////////////
	// Placement New ����
	///////////////////////////////////////////////////////////////////////
	bool				_bPlacementNew;
	
};

#endif