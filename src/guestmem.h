#ifndef GUESTMEM_H
#define GUESTMEM_H

#include <iostream>
#include <list>
#include <map>

class GuestMem
{
public:

/* extent of guest memory range; may need to extend later so 
 * getData works on non-identity mappings */
class Mapping {
public:
	Mapping(void) {}
	Mapping(void* in_off, size_t in_len, int prot, bool in_is_stack=false)
	: offset(in_off)
	, length(in_len)
	, req_prot(prot)
	, cur_prot(prot)
	, is_stack(in_is_stack) {}
	virtual ~Mapping(void) {}

	void* end() const { return (char*)offset + length; }
	
	const void* getData(void) const { return offset; }
	const void* getGuestAddr(void) const { return offset; }
	unsigned int getBytes(void) const { return length; }
	bool isStack(void) const { return is_stack; }
	int getReqProt(void) const { return req_prot; }
	int getCurProt(void) const { return cur_prot; }
	void print(std::ostream& os) const;

	void* 		offset;
	size_t		length;
	int		req_prot;
	int		cur_prot;
	bool		is_stack;
};

	GuestMem(void);
	virtual ~GuestMem(void);
	void recordMapping(Mapping& mapping);
	void removeMapping(Mapping& mapping);
	bool lookupMapping(void* addr, Mapping& mapping);
	void* brk();
	bool sbrk(void* new_top);
	std::list<Mapping> getMaps(void) const;
	
private:
	typedef std::map<void*, Mapping> mapmap_t; 
	mapmap_t maps;
	void* top_brick;
};

#endif