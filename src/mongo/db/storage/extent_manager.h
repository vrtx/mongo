// extent_manager.h

/**
*    Copyright (C) 2013 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#pragma once

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "mongo/base/status.h"
#include "mongo/base/string_data.h"
#include "mongo/db/diskloc.h"

namespace mongo {

    class DataFile;

    /**
     * ExtentManager basics
     *  - one per database
     *  - responsible for managing <db>.# files
     *  - NOT responsible for .ns file
     *  - gives out extents
     *  - responsible for figuring out how to get a new extent
     *  - can use any method it wants to do so
     *  - this structure is NOT stored on disk
     *  - this class is NOT thread safe, locking should be above (for now)
     *
     * implementation:
     *  - ExtentManager holds a list of DataFile
     */
    class ExtentManager {
        MONGO_DISALLOW_COPYING( ExtentManager );

    public:
        /**
         * @param freeListDetails this is a reference into the .ns file
         *        while a bit odd, this is not a layer violation as extents
         *        are a peer to the .ns file, without any layering
         */
        ExtentManager( const StringData& dbname, const StringData& path,
                       bool directoryPerDB );

        ~ExtentManager();

        /**
         * deletes all state and puts back to original state
         */
        void reset();

        /**
         * opens all current files
         */
        Status init();

        size_t numFiles() const;
        long long fileSize() const;

        // TODO: make private
        DataFile* getFile( int n, int sizeNeeded = 0, bool preallocateOnly = false );

        // TODO(ERH): remove?
        void preallocateAFile() { getFile( numFiles() , 0, true ); }

        void flushFiles( bool sync );

        // must call Extent::reuse on the returned extent
        DiskLoc allocateExtent( const string& ns,
                                bool capped,
                                int size,
                                int quotaMax );

        /**
         * firstExt has to be == lastExt or a chain
         */
        void freeExtents( DiskLoc firstExt, DiskLoc lastExt );

        void printFreeList() const;

        void freeListStats( int* numExtents, int64_t* totalFreeSize ) const;

        /**
         * @param loc - has to be for a specific Record
         * Note(erh): this sadly cannot be removed.
         * A Record DiskLoc has an offset from a file, while a RecordStore really wants an offset
         * from an extent.  This intrinsically links an original record store to the original extent
         * manager.
         */
        Record* recordForV1( const DiskLoc& loc ) const;

        /**
         * @param loc - has to be for a specific Record (not an Extent)
         * Note(erh) see comment on recordFor
         */
        Extent* extentForV1( const DiskLoc& loc ) const;

        /**
         * @param loc - has to be for a specific Record (not an Extent)
         * Note(erh) see comment on recordFor
         */
        DiskLoc extentLocForV1( const DiskLoc& loc ) const;

        /**
         * @param loc - has to be for a specific Extent
         */
        Extent* getExtent( const DiskLoc& loc, bool doSanityCheck = true ) const;

        Extent* getNextExtent( Extent* ) const;
        Extent* getPrevExtent( Extent* ) const;

        // get(Next|Prev)Record follows the Record linked list
        // these WILL cross Extent boundaries
        // * @param loc - has to be the DiskLoc for a Record

        // * TODO(ERH): remove this - only RecordStore can do this
        DiskLoc getNextRecord( const DiskLoc& loc ) const;

        // * TODO(ERH): remove this - only RecordStore can do this
        DiskLoc getPrevRecord( const DiskLoc& loc ) const;

        // does NOT traverse extent boundaries

        // * TODO(ERH): remove this - only RecordStore can do this
        DiskLoc getNextRecordInExtent( const DiskLoc& loc ) const;

        // * TODO(ERH): remove this - only RecordStore can do this
        DiskLoc getPrevRecordInExtent( const DiskLoc& loc ) const;

        /**
         * quantizes extent size to >= min + page boundary
         */
        static int quantizeExtentSize( int size );

    private:

        /**
         * will return NULL if nothing suitable in free list
         */
        DiskLoc _allocFromFreeList( int approxSize, bool capped );

        /* allocate a new Extent, does not check free list
         * @param maxFileNoForQuota - 0 for unlimited
        */
        DiskLoc _createExtent( int approxSize, int maxFileNoForQuota );

        DataFile* _addAFile( int sizeNeeded, bool preallocateNextFile );

        DiskLoc _getFreeListStart() const;
        DiskLoc _getFreeListEnd() const;
        void _setFreeListStart( DiskLoc loc );
        void _setFreeListEnd( DiskLoc loc );

        const DataFile* _getOpenFile( int n ) const;

        DiskLoc _createExtentInFile( int fileNo, DataFile* f,
                                     int size, int maxFileNoForQuota );

        boost::filesystem::path fileName( int n ) const;

// -----

        std::string _dbname; // i.e. "test"
        std::string _path; // i.e. "/data/db"
        bool _directoryPerDB;

        // must be in the dbLock when touching this (and write locked when writing to of course)
        // however during Database object construction we aren't, which is ok as it isn't yet visible
        //   to others and we are in the dbholder lock then.
        std::vector<DataFile*> _files;

    };

}
