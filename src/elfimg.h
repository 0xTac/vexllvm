#ifndef ELFIMG_H
#define ELFIMG_H

#include <elf.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <map>
#include "collection.h"

#include "symbols.h"

typedef void* hostptr_t;
typedef void* elfptr_t;
typedef const void* celfptr_t;

class ElfSegment;
class DLLib;

class ElfImg
{
public:
	typedef std::map<std::string, void*> symmap;
	static ElfImg* create(const char* fname);
	static ElfImg* createUnlinked(const char* fname);
	virtual ~ElfImg(void);
	bool isValid(void) const { return fd > 0; }
	hostptr_t xlateAddr(elfptr_t addr) const;
	elfptr_t getEntryPoint(void) const;
	int getHeaderCount() const;
	ElfImg* getInterp(void) const { return interp; }
	bool isDirectMapped(void) const { return direct_mapped; } 
	elfptr_t getSymAddr(const char* symname) const;
	const char* getFilePath(void) const { return img_path; }
	void linkIt(void);
	celfptr_t getHeader() const;
	celfptr_t getBase() const;
	ElfSegment* getFirstSegment() const { return segments.front(); }
	void getSegments(std::list<ElfSegment*>& r) const;
private:
	ElfImg(const char* fname, bool linked);
	bool verifyHeader(void) const;
	void setupSegments(void);
	void applyRelocs(void);
	void applyRelaSection(const Elf64_Shdr& shdr);
	void applyRela(const Elf64_Rela& shdr);
	void loadDyn(void);
	const char* getStr(unsigned int stroff) const;
	hostptr_t* getSectPtr(const Elf64_Shdr* shdr) const;
	unsigned int getSectElems(const Elf64_Shdr* shdr) const;
	const Elf64_Shdr* getDynShdr(void) const;
	const Elf64_Sym* getSym(unsigned int symidx) const;
	const char* getDynStr(unsigned int i) const {return &dynstr_tab[i];}

	void pullInstrumented(DLLib* lib);
	void linkWithLibs(std::vector<std::string>& needed);
	void linkWith(DLLib* lib);
	void* getLinkValue(const char* symname) const;

	char			*img_path;

	PtrList<ElfSegment>	segments;
	void			*img_mmap;
	unsigned int		img_bytes_c;
	int			fd;
	const Elf64_Ehdr	*hdr;
	const Elf64_Shdr	*shdr_tab;

	const Elf64_Shdr*	dynsym_shdr;
	const Elf64_Sym*	dynsym_tab; 
	unsigned int		dynsym_c;
	const char		*dynstr_tab;
	Symbols			syms;
	PtrList<DLLib>		libs;	/* all libs linked in XXX make obj */

	bool direct_mapped;
	ElfImg* interp;
	bool linked;
};

#endif
