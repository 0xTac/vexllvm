#ifndef GUESTMEM_H
#define GUESTMEM_H

#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include "collection.h"
#include "guestptr.h"

/* oh good, MAP_32BIT isn't defined in the ARM headers */
#if defined(__arm__)
#define MAP_32BIT 0
#endif

class GuestMem
{
public:

/* extent of guest memory range */
class Mapping
{
public:
	enum MapType { REG = 0, STACK = 1, HEAP = 2, UNMAPPED = 3, VSYSPAGE = 4 };

	Mapping(void) : offset(0), length(0), type(REG) {}

	Mapping(guest_ptr in_off, size_t in_len, int prot,
		const std::string* _name=0)
	: offset(in_off)
	, length(in_len)
	, req_prot(prot)
	, cur_prot(prot)
	, type(REG)
	, name(_name) {}

	virtual ~Mapping(void) {}

	guest_ptr end() const { return offset + length; }

	bool contains(guest_ptr p) const
	{ return (p >= offset && p < offset + length); }

	unsigned int getBytes(void) const { return length; }
	int getReqProt(void) const { return req_prot; }
	int getCurProt(void) const { return cur_prot; }
	void print(std::ostream& os) const;

	std::string getName(void) const { return (name) ? *name : ""; }

	bool isValid() const { return offset && length; }

	guest_ptr		offset;
	size_t			length;
	int			req_prot;
	int			cur_prot;
	MapType			type;
	const std::string	*name;
};

	GuestMem(void);
	virtual ~GuestMem(void);

	void nameMapping(guest_ptr addr, const std::string& s);
	void recordMapping(Mapping& mapping);
	bool lookupMapping(guest_ptr addr, Mapping& mapping);
	std::list<Mapping> getMaps(void) const;
	void setType(guest_ptr addr, Mapping::MapType);

	void mark32Bit() { is_32_bit = true; }
	bool is32Bit() const { return is_32_bit; }

	char* getBase() const { return base; }
	bool findFreeRegion(size_t len, Mapping& m) const;
	bool isFlat() const { return force_flat; }
	void print(std::ostream &os) const;

	void* getData(const Mapping& m) const { return (void*)(base + m.offset.o); }

	/* access small bits of guest memory */
	template <typename T>
	T read(guest_ptr offset) const { return *(T*)(base + offset.o); }
	uintptr_t readNative(guest_ptr offset, int idx = 0) {
		if(is_32_bit)
			return *(uint32_t*)(base + offset.o + idx*4);
		else
			return *(uint64_t*)(base + offset.o + idx*8);
	}
	template <typename T>
	void write(guest_ptr offset, const T& t) {
		*(T*)(base + offset.o) = t;
	}
	void writeNative(guest_ptr offset, uintptr_t t) {
		if(is_32_bit)
			*(unsigned int*)(base + offset.o) = t;
		else
			*(unsigned long*)(base + offset.o) = t;
	}

	/* access large blocks of guest memory */
	void memcpy(guest_ptr dest, const void* src, size_t len) {
		::memcpy(base + dest.o, src, len);
	}
	void memcpy(void* dest, guest_ptr src, size_t len) const {
		::memcpy(dest, base + src.o, len);
	}
	void memset(guest_ptr dest, char d, size_t len) {
		::memset(base + dest.o, d, len);
	}
	int strlen(guest_ptr p) const {
		return ::strlen(base + p.o);
	}
	void* getHostPtr(guest_ptr p) const { return (void*)(base + p.o); }

	/* virtual memory handling, these update the mappings as
	   necessary and also do the proper protection to
	   distinguish between self-modifying/generating code
	   and normal data writes */
	guest_ptr brk() const { return top_brick; }
	bool sbrk(guest_ptr new_top);
	int mmap(guest_ptr& result, guest_ptr addr, size_t length,
	 	int prot, int flags, int fd, off_t offset);
	int mprotect(guest_ptr offset, size_t length,
		int prot);
	int munmap(guest_ptr offset, size_t length);
	int mremap(
		guest_ptr& result, guest_ptr old_offset,
		size_t old_length, size_t new_length,
		int flags, guest_ptr new_offset);

	const void* getSysHostAddr(guest_ptr p) const;
	void addSysPage(guest_ptr p, char* host_data, unsigned int len);

private:
	bool sbrkInitial();
	bool sbrkInitial(guest_ptr new_top);

	void removeMapping(Mapping& mapping);

	const Mapping* lookupMapping(guest_ptr addr) const;
	const Mapping* findNextMapping(guest_ptr addr) const;
	const Mapping* findOwner(guest_ptr addr) const;
	bool findFreeRegionByMaps(size_t len, Mapping& m) const;

	bool canUseRange(guest_ptr base, unsigned int len) const;

	typedef std::map<guest_ptr, Mapping*> mapmap_t;
	mapmap_t	maps;
	char*		base;

	guest_ptr	top_brick;
	guest_ptr	base_brick;
	guest_ptr	reserve_brick;

	bool		is_32_bit;
	bool		force_flat;

	char*		syspage_data;

	PtrList<std::string>	mapping_names;
};

#endif
