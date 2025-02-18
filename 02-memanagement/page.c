#include "os.h"

/*
 * Following global vars are defined in mem.S
 */
extern ptr_t TEXT_START;
extern ptr_t TEXT_END;
extern ptr_t DATA_START;
extern ptr_t DATA_END;
extern ptr_t RODATA_START;
extern ptr_t RODATA_END;
extern ptr_t BSS_START;
extern ptr_t BSS_END;
extern ptr_t HEAP_START;
extern ptr_t HEAP_SIZE;

/*
 * _alloc_start points to the actual start address of heap pool
 * _alloc_end points to the actual end address of heap pool
 * _num_pages holds the actual max number of pages we can allocate.
 */
static ptr_t _alloc_start = 0;
static ptr_t _alloc_end = 0;
static uint32_t _num_pages = 0;

#define PAGE_SIZE 4096
#define PAGE_ORDER 12

#define PAGE_TAKEN (uint8_t)(1 << 0)
#define PAGE_LAST  (uint8_t)(1 << 1)

#define BYTE_TAKEN (uint8_t)(1 << 0)
#define BYTE_LAST  (uint8_t)(1 << 1)

/*
 * Page Descriptor 
 * flags:
 * - bit 0: flag if this page is taken(allocated)
 * - bit 1: flag if this page is the last page of the memory block allocated
 */
struct Page {
	uint8_t flags;
	uint16_t num_byte_allocated; // 0~4096
	int last_allocated; // 0~4095
	uint8_t bitmap[4096];  // to be optimized
};

static inline void _clear(struct Page *page)
{
	page->flags = 0;
}

static inline void _clear_byte(struct Page *page, int index)
{
	page->bitmap[index] = 0;
}

static inline int _is_free(struct Page *page)
{
	if (page->flags & PAGE_TAKEN) {
		return 0;
	} else {
		return 1;
	}
}

static inline int _is_free_byte(struct Page *page, int index)
{
	if (page->bitmap[index] & BYTE_TAKEN) {
		return 0;
	} else {
		return 1;
	}
}

static inline void _set_flag(struct Page *page, uint8_t flags)
{
	page->flags |= flags;
}

static inline void _set_flag_byte(struct Page *page, uint8_t byte_flags, int index)
{
	page->bitmap[index] |= byte_flags;
}

static inline int _is_last(struct Page *page)
{
	if (page->flags & PAGE_LAST) {
		return 1;
	} else {
		return 0;
	}
}

static inline int _is_last_byte(struct Page *page, int index)
{
	if (page->bitmap[index] & BYTE_LAST) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * align the address to the border of page(4K)
 */
static inline ptr_t _align_page(ptr_t address)
{
	ptr_t order = (1 << PAGE_ORDER) - 1;
	return (address + order) & (~order);
}

/*
 *    ______________________________HEAP_SIZE_______________________________
 *   /   ___num_reserved_pages___   ______________num_pages______________   \
 *  /   /                        \ /                                     \   \
 *  |---|<--Page-->|<--Page-->|...|<--Page-->|<--Page-->|......|<--Page-->|---|
 *  A   A                         A                                       A   A
 *  |   |                         |                                       |   |
 *  |   |                         |                                       |   _memory_end
 *  |   |                         |                                       |
 *  |   _heap_start_aligned       _alloc_start                            _alloc_end
 *  HEAP_START(BSS_END)
 *
 *  Note: _alloc_end may equal to _memory_end.
 */
void page_init()
{
	ptr_t _heap_start_aligned = _align_page(HEAP_START);

	/* 
	 * We reserved some Pages to hold the Page structures.
	 * The number of reserved pages depends on the LENGTH_RAM.
	 * For simplicity, the space we reserve here is just an approximation,
	 * assuming that it can accommodate the maximum LENGTH_RAM.
	 * We assume LENGTH_RAM should not be too small, ideally no less
	 * than 16M (i.e. PAGE_SIZE * PAGE_SIZE).
	 */
	// uint32_t num_reserved_pages = LENGTH_RAM / (PAGE_SIZE * PAGE_SIZE);
	uint32_t num_reserved_pages = 17000; // to be optimized

	_num_pages = (HEAP_SIZE - (_heap_start_aligned - HEAP_START))/ PAGE_SIZE - num_reserved_pages;
	printf("HEAP_START = %p(aligned to %p), HEAP_SIZE = 0x%lx,\n"
	       "num of reserved pages = %d, num of pages to be allocated for heap = %d\n",
	       HEAP_START, _heap_start_aligned, HEAP_SIZE,
	       num_reserved_pages, _num_pages);
	
	/*
	 * We use HEAP_START, not _heap_start_aligned as begin address for
	 * allocating struct Page, because we have no requirement of alignment
	 * for position of struct Page.
	 */
	struct Page *page = (struct Page *)HEAP_START;
	for (int i = 0; i < _num_pages; i++) {
		_clear(page);
		page++;	
	}

	_alloc_start = _heap_start_aligned + num_reserved_pages * PAGE_SIZE;
	_alloc_end = _alloc_start + (PAGE_SIZE * _num_pages);

	printf("TEXT:   %p -> %p\n", TEXT_START, TEXT_END);
	printf("RODATA: %p -> %p\n", RODATA_START, RODATA_END);
	printf("DATA:   %p -> %p\n", DATA_START, DATA_END);
	printf("BSS:    %p -> %p\n", BSS_START, BSS_END);
	printf("HEAP:   %p -> %p\n", _alloc_start, _alloc_end);
}

/*
 * Allocate a memory block which is composed of contiguous physical pages
 * - npages: the number of PAGE_SIZE pages to allocate
 */
void *page_alloc(int npages)
{
	/* Note we are searching the page descriptor bitmaps. */
	int found = 0;
	struct Page *page_i = (struct Page *)HEAP_START;
	for (int i = 0; i <= (_num_pages - npages); i++) {
		if (_is_free(page_i)) {
			found = 1;
			/* 
			 * meet a free page, continue to check if following
			 * (npages - 1) pages are also unallocated.
			 */
			struct Page *page_j = page_i + 1;
			for (int j = i + 1; j < (i + npages); j++) {
				if (!_is_free(page_j)) {
					found = 0;
					break;
				}
				page_j++;
			}
			/*
			 * get a memory block which is good enough for us,
			 * take housekeeping, then return the actual start
			 * address of the first page of this memory block
			 */
			if (found) {
				struct Page *page_k = page_i;
				for (int k = i; k < (i + npages); k++) {
					_set_flag(page_k, PAGE_TAKEN);
					page_k->num_byte_allocated = 0;
					page_k->last_allocated = -1;
					page_k++;
				}
				page_k--;
				_set_flag(page_k, PAGE_LAST);
				page_k->num_byte_allocated = 0;
				page_k->last_allocated = -1;
				return (void *)(_alloc_start + i * PAGE_SIZE);
			}
		}
		page_i++;
	}
	return NULL;
}

void *malloc(size_t size) {
	struct Page *page = (struct Page *)HEAP_START;
	if (size < PAGE_SIZE) {
		for (int i = 0; i < _num_pages; i++) {
			struct Page *page_i = page + i;
			if (!_is_free(page_i)) {
				if ((page_i->last_allocated == -1) || (size < PAGE_SIZE - page_i->last_allocated)) {
					int j;
					for (j = page_i->last_allocated + 1; j <= page_i->last_allocated + size; j++) {
						_set_flag_byte(page_i, BYTE_TAKEN, j);
					}
					j--;
					page_i->last_allocated = j;
					_set_flag_byte(page_i, BYTE_LAST, j);
					page_i->num_byte_allocated += size;
					return (void *)(_alloc_start + i * PAGE_SIZE + page_i->last_allocated - size + 1);
				}
			}
			else {
				_set_flag(page_i, PAGE_TAKEN);
				int j;
				for (j = 0; j < size; j++)
					_set_flag_byte(page_i, BYTE_TAKEN, j);
				j--;
				page_i->last_allocated = j;
				_set_flag_byte(page_i, BYTE_LAST, j);
				page_i->num_byte_allocated += size;
				return (void *)(_alloc_start + i * PAGE_SIZE);
			}
		}
	}
	int npages = size / PAGE_SIZE;
	int last_bytes = size % PAGE_TAKEN;
	void *ret = page_alloc(npages + 1);
	int page_index = (int)(ret - _alloc_start) / PAGE_SIZE;
	page += page_index;
	for (int i = 0; i < npages; i++) {
		struct Page *page_i = page + i;
		for (int j = 0; j < PAGE_SIZE; j++) {
			_set_flag_byte(page_i, BYTE_TAKEN, j);
		}
		page_i->num_byte_allocated = 4096;
		page_i->last_allocated = 4095;
	}
	struct Page *page_last = page + npages;
	int j;
	for (j = 0; j <last_bytes; j++) {
		_set_flag_byte(page_last, BYTE_TAKEN, j);
	}
	j--;
	page_last->last_allocated = j;
	_set_flag_byte(page_last, BYTE_LAST, j);
	page_last->num_byte_allocated = last_bytes;
	return ret;
}

/*
 * Free the memory block
 * - p: start address of the memory block
 */
void page_free(void *p)
{
	/*
	 * Assert (TBD) if p is invalid
	 */
	if (!p || (ptr_t)p >= _alloc_end) {
		return;
	}
	/* get the first page descriptor of this memory block */
	struct Page *page = (struct Page *)HEAP_START;
	page += ((ptr_t)p - _alloc_start)/ PAGE_SIZE;
	/* loop and clear all the page descriptors of the memory block */
	while (!_is_free(page)) {
		if (_is_last(page)) {
			_clear(page);
			break;
		} else {
			_clear(page);
			page++;;
		}
	}
}

void free(void *ptr) {
	if (!ptr || (ptr_t)ptr >= _alloc_end) {
		return;
	}
	struct Page *page = (struct Page *)HEAP_START;
	page += ((ptr_t)ptr - _alloc_start)/ PAGE_SIZE;
	int start = (int)(((ptr_t)ptr - _alloc_start)) % PAGE_SIZE;
	while (1) {
		int i;
		for (i = start; i < PAGE_SIZE; i++) {
			if (!_is_last_byte(page, i)) {
				_clear_byte(page, i);
				page->num_byte_allocated--;
			}
			else {
				_clear_byte(page, i);
				page->num_byte_allocated--;
				if (page->num_byte_allocated == 0) {
					page->last_allocated = -1;
					_clear(page);
				}
				return;
			}
		}
		if (i >= PAGE_SIZE) {
			if (page->num_byte_allocated == 0) {
				page->last_allocated = -1;
				_clear(page);
			}
			page++;
			start = 0;
		}
	}
}

void page_test()
{
	void *p = page_alloc(2);
	printf("p = %p\n", p);
	//page_free(p);

	void *p2 = page_alloc(7);
	printf("p2 = %p\n", p2);
	page_free(p2);

	void *p3 = page_alloc(4);
	printf("p3 = %p\n", p3);
}

void malloc_test() {
	void *p0 = page_alloc(2);
	printf("p0 = %p\n", p0);

	void *p1 = page_alloc(2);
	printf("p1 = %p\n", p1);

	void *pm0 = malloc(1024);
	printf("pm0 = %p\n", pm0);

	void *pm1 = malloc(2048);
	printf("pm1 = %p\n", pm1);

	void *pm2 = malloc(6144);
	printf("pm2 = %p\n", pm2);

	free(pm0);
	void *pm3 = malloc(1024);
	printf("pm3 = %p\n", pm3);

	free(pm1);
	free(pm3);
	void *pm4 = malloc(1024);
	printf("pm4 = %p\n", pm4);
}
