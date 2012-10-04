#pragma once

#include "mongo/pch.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/diskloc.h"
#include "mongo/db/pdfile.h"
#include "mongo/db/key.h"
#include <stdint.h>

#define RADIX_ARENA_SIZE 1024 * 1024

namespace mongo {

	// Radix Arena
	//////////////
#pragma pack(1)
	class RadixKeyArena {
	public:

		explicit RadixKeyArena(const uint32_t size) : 
			_size(size - 4),
			_startLoc(),
			_bytesFree(size - 4) { }

		static RadixKeyArena *load(const DiskLoc &arenaLoc) {
			return reinterpret_cast <RadixKeyArena *>(arenaLoc.rec()->data());
		}

		static RadixKeyArena *allocate(const char *ns, const uint32_t size) {
			DiskLoc startLoc = theDataFileMgr.insert(ns, 0, size, true, false);
			RadixKeyArena *newArena = new(getDur().writingPtr(startLoc.rec()->data() + 4, sizeof(newArena))) 
										RadixKeyArena(size);
			newArena->_startLoc = startLoc;
			log() << "1 startLoc: "<< startLoc << endl;
			return newArena;
		}

		const char *addKeyData(const char *keyData, uint32_t length) {
			if (length > _bytesFree)
				return NULL;
			log() << "2 startLoc: "<< _startLoc << endl;
			log() << "size: " << _size << endl;
			log() << "bytesFree: " << _bytesFree << endl;
			log() << "writePos: " << _size - _bytesFree + 4 << endl;
			log() << "  startLocData: " << _startLoc.rec()->data() << endl;
			char *durKeyData = reinterpret_cast <char *>(getDur().writingPtr(
									_startLoc.rec()->data() + (_size - _bytesFree) + 4 + sizeof(RadixKeyArena), length));
			memcpy(durKeyData, keyData, length);
			_bytesFree -= length;
			return durKeyData;
		}

		char *getKeyData(uint32_t offset) const {
			return _startLoc.rec()->data() + 4 + sizeof(RadixKeyArena) + offset;
		}

		void dumpKeyData() const {
			log() << "Key Data: " << getKeyData(0) << endl;
		}

		static void test(const char *ns) {
			// string ns = id.indexNamespace();
			RadixKeyArena *arena = RadixKeyArena::allocate(ns, RADIX_ARENA_SIZE);
			log() << arena->addKeyData("Testing", 7) << endl;
			log() << arena->addKeyData("Radix.Test Collection", 21) << endl;
			arena->dumpKeyData();
			LOG(0) << "done." << endl;
		}

	private:
		RadixKeyArena();
		RadixKeyArena(const RadixKeyArena& rhs);

		// header
		uint32_t _size;
		DiskLoc _startLoc;
		uint32_t _bytesFree;	// todo: freelist
	};
#pragma pack()

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

}
