#pragma once

#include "mongo/pch.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/diskloc.h"
#include "mongo/db/pdfile.h"
#include "mongo/db/key.h"

#define RADIX_ARENA_SIZE 1024 * 1024;

// Radix Arena
//////////////
#pragma pack(1)
class RadixKeyArena {
public:
	explicit RadixKeyArena(const uint32_t size) : 
		_startLoc(DiskLoc),
		_size(size),
		_bytesFree(size) { 
	}

	static RadixKeyArena *load(const DiskLoc &arenaLoc) {
		return static_cast <RadixKeyArena *>(arenaLoc.rec().data());
	}

	static RadixKeyArena *allocate(const char *ns, const uint32_t size) {
		DiskLoc startLoc = theDataFileMgr.insert(ns, 0, size, true);
		RadixKeyArena *newArena = new(getDur().writingPtr(startLoc.rec().data(), sizeof(newArena))) 
									RadixKeyArena(size);
		newArena->_startLoc = startLoc;
		return newArena;
	}

	const char *addKeyData(const char *keyData, uint32_t length) {
		if (length > _bytesFree)
			return NULL;
		char *durKeyData = getDur().writingPtr(_startLoc.rec().data() + (_size - _bytesFree), length);
		memcpy(durKeyData, keyData, length);
		getDur().writingPtr(this, sizeof(RadixKeyArena));
		_bytesFree -= length;
		return durKeyData;
	}

	char *getKeydata(uint32_t offset) const {
		return _startLoc.rec().data() + offset;
	}

	void dumpKeyData() const {
		log() << "Key Data: " << getKeyData(0) << endl;
	}

	static void test() {
		ns = id.indexNamespace();
		RadixKeyArena *arena = RadixKeyArena::allocate();
		arena->addKeyData("Testing", 7);
		arena->dumpKeyData();
	}

private:
	RadixKeyArena();		// hidden: RadixKeyArena is alloc'd in the context of a namespace

	DiskLoc _startLoc;
	uint32_t _size;
	uint32_t _bytesFree;	// todo: freelist
};

// // Radix Tree
// /////////////
// class RadixTree {
// public:
// 	RadixTree() { }
// 	void init(const IndexDetails& id) {
//         ns = id.indexNamespace();
// 	}

// private:
// 	void allocKeyArena(uint64_t arenaSize) {
// 		_arenas.insert(RadixKeyArena(arenaSize));
// 	}
// 	vector<RadixKeyArena> _arenas;
// 	const string _ns;
// };

// class RadixNode;
// class RadixEdge;
// class RadixTreeCursor;


#pragma pack
