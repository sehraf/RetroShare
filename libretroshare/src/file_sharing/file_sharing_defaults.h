#pragma once

static const uint32_t DELAY_BETWEEN_DIRECTORY_UPDATES           = 100 ; // 10 seconds for testing. Should be much more!!
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ   = 10 ; // 10 seconds for testing. Should be much more!!
static const uint32_t DELAY_BETWEEN_LOCAL_DIRECTORIES_TS_UPDATE = 10 ; // 10 seconds for testing. Should be much more!!

static const std::string HASH_CACHE_DURATION_SS = "HASH_CACHE_DURATION" ;	// key string to store hash remembering time
static const std::string WATCH_FILE_DURATION_SS = "WATCH_FILES_DELAY" ;		// key to store delay before re-checking for new files

static const std::string FILE_SHARING_DIR_NAME  = "file_sharing" ;			// hard-coded directory name to store friend file lists, hash cache, etc.
static const std::string HASH_CACHE_FILE_NAME   = "hash_cache.bin" ;		// hard-coded directory name to store encrypted hash cache.

static const uint32_t MIN_INTERVAL_BETWEEN_HASH_CACHE_SAVE         = 20 ;  // never save hash cache more often than every 20 secs.
static const uint32_t MIN_INTERVAL_BETWEEN_REMOTE_DIRECTORY_SAVE   = 23 ;  // never save remote directories more often than this