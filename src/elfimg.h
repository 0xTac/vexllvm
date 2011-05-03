#ifndef ELFIMG_H
#define ELFIMG_H

#include <elf.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <map>
#include "collection.h"

typedef void* hostptr_t;
typedef void* elfptr_t;

class ElfSegment;
class DLLib;

class ElfImg
{
public:
typedef std::map<std::string, void*> symmap;
	static ElfImg* create(const char* fname);
	virtual ~ElfImg(void);
	bool isValid(void) const { return fd > 0; }
	hostptr_t xlateAddr(elfptr_t addr) const;
	elfptr_t getEntryPoint(void) const { return (void*)hdr->e_entry; }
private:
	ElfImg(const char* fname);
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

	void linkWithLibs(std::vector<std::string>& needed);
	void linkWith(DLLib* lib);
	void* getLinkValue(const char* symname) const;

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
	symmap			sym_map;
	PtrList<DLLib>		libs;	/* all libs linked in XXX make obj */
};

#endif
