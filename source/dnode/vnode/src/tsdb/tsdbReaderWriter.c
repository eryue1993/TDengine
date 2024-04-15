/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cos.h"
#include "tsdb.h"
<<<<<<< HEAD
#include "crypt.h"
=======
#include "vnd.h"
>>>>>>> 3.0

static int32_t tsdbOpenFileImpl(STsdbFD *pFD) {
  int32_t     code = 0;
  const char *path = pFD->path;
  int32_t     szPage = pFD->szPage;
  int32_t     flag = pFD->flag;
  int64_t     lc_size = 0;

  pFD->pFD = taosOpenFile(path, flag);
  if (pFD->pFD == NULL) {
    if (tsS3Enabled && pFD->lcn > 1 && !strncmp(path + strlen(path) - 5, ".data", 5)) {
      char lc_path[TSDB_FILENAME_LEN];
      tstrncpy(lc_path, path, TSDB_FQDN_LEN);

      char *dot = strrchr(lc_path, '.');
      if (!dot) {
        tsdbError("unexpected path: %s", lc_path);
        code = TAOS_SYSTEM_ERROR(ENOENT);
        goto _exit;
      }
      snprintf(dot + 1, TSDB_FQDN_LEN - (dot + 1 - lc_path), "%d.data", pFD->lcn);

      pFD->pFD = taosOpenFile(lc_path, flag);
      if (pFD->pFD == NULL) {
        code = TAOS_SYSTEM_ERROR(errno);
        // taosMemoryFree(pFD);
        goto _exit;
      }
      if (taosStatFile(lc_path, &lc_size, NULL, NULL) < 0) {
        code = TAOS_SYSTEM_ERROR(errno);
        // taosCloseFile(&pFD->pFD);
        // taosMemoryFree(pFD);
        goto _exit;
      }
    } else {
      tsdbInfo("no file: %s", path);
      code = TAOS_SYSTEM_ERROR(errno);
      // taosMemoryFree(pFD);
      goto _exit;
    }
    /*
    const char *object_name = taosDirEntryBaseName((char *)path);
    long        s3_size = 0;
    if (tsS3Enabled) {
      long size = s3Size(object_name);
      if (size < 0) {
        code = terrno = TSDB_CODE_FAILED_TO_CONNECT_S3;
        goto _exit;
      }

      s3_size = size;
    }

    if (tsS3Enabled && !strncmp(path + strlen(path) - 5, ".data", 5) && s3_size > 0) {
#ifndef S3_BLOCK_CACHE
      s3EvictCache(path, s3_size);
      s3Get(object_name, path);

      pFD->pFD = taosOpenFile(path, flag);
      if (pFD->pFD == NULL) {
        code = TAOS_SYSTEM_ERROR(ENOENT);
        // taosMemoryFree(pFD);
        goto _exit;
      }
#else
      pFD->s3File = 1;
      pFD->pFD = (TdFilePtr)&pFD->s3File;
      int32_t vid = 0;
      sscanf(object_name, "v%df%dver%" PRId64 ".data", &vid, &pFD->fid, &pFD->cid);
      pFD->objName = object_name;
      // pFD->szFile = s3_size;
#endif
    */
  }

  pFD->pBuf = taosMemoryCalloc(1, szPage);
  if (pFD->pBuf == NULL) {
    code = TSDB_CODE_OUT_OF_MEMORY;
    // taosCloseFile(&pFD->pFD);
    // taosMemoryFree(pFD);
    goto _exit;
  }

  if (lc_size > 0) {
    SVnodeCfg *pCfg = &pFD->pTsdb->pVnode->config;
    int64_t    chunksize = (int64_t)pCfg->tsdbPageSize * pCfg->s3ChunkSize;

    pFD->szFile = lc_size + chunksize * (pFD->lcn - 1);
  }

  // not check file size when reading data files.
  if (flag != TD_FILE_READ /* && !pFD->s3File*/) {
    if (!lc_size && taosStatFile(path, &pFD->szFile, NULL, NULL) < 0) {
      code = TAOS_SYSTEM_ERROR(errno);
      // taosMemoryFree(pFD->pBuf);
      // taosCloseFile(&pFD->pFD);
      // taosMemoryFree(pFD);
      goto _exit;
    }
  }

  ASSERT(pFD->szFile % szPage == 0);
  pFD->szFile = pFD->szFile / szPage;

_exit:
  return code;
}

// =============== PAGE-WISE FILE ===============
int32_t tsdbOpenFile(const char *path, STsdb *pTsdb, int32_t flag, STsdbFD **ppFD, int32_t lcn) {
  int32_t  code = 0;
  STsdbFD *pFD = NULL;
  int32_t  szPage = pTsdb->pVnode->config.tsdbPageSize;

  *ppFD = NULL;

  pFD = (STsdbFD *)taosMemoryCalloc(1, sizeof(*pFD) + strlen(path) + 1);
  if (pFD == NULL) {
    code = TSDB_CODE_OUT_OF_MEMORY;
    goto _exit;
  }

  pFD->path = (char *)&pFD[1];
  strcpy(pFD->path, path);
  pFD->szPage = szPage;
  pFD->flag = flag;
  pFD->szPage = szPage;
  pFD->pgno = 0;
  pFD->lcn = lcn;
  pFD->pTsdb = pTsdb;

  *ppFD = pFD;

_exit:
  return code;
}

void tsdbCloseFile(STsdbFD **ppFD) {
  STsdbFD *pFD = *ppFD;
  if (pFD) {
    taosMemoryFree(pFD->pBuf);
    // if (!pFD->s3File) {
    taosCloseFile(&pFD->pFD);
    //}
    taosMemoryFree(pFD);
    *ppFD = NULL;
  }
}

static int32_t tsdbWriteFilePage(STsdbFD *pFD, int32_t encryptAlgorithm, char* encryptKey) {
  int32_t code = 0;

  if (!pFD->pFD) {
    code = tsdbOpenFileImpl(pFD);
    if (code) {
      goto _exit;
    }
  }

  if (pFD->pgno > 0) {
    int64_t offset = PAGE_OFFSET(pFD->pgno, pFD->szPage);
    if (pFD->lcn > 1) {
      SVnodeCfg *pCfg = &pFD->pTsdb->pVnode->config;
      int64_t    chunksize = (int64_t)pCfg->tsdbPageSize * pCfg->s3ChunkSize;
      int64_t    chunkoffset = chunksize * (pFD->lcn - 1);

      offset -= chunkoffset;
    }
    ASSERT(offset >= 0);

    int64_t n = taosLSeekFile(pFD->pFD, offset, SEEK_SET);
    if (n < 0) {
      code = TAOS_SYSTEM_ERROR(errno);
      goto _exit;
    }

    taosCalcChecksumAppend(0, pFD->pBuf, pFD->szPage);
    
    if(encryptAlgorithm == DND_CA_SM4){
    //if(tsiEncryptAlgorithm == DND_CA_SM4 && (tsiEncryptScope & DND_CS_TSDB) == DND_CS_TSDB){
      unsigned char		PacketData[128];
      int		NewLen;
      int32_t count = 0;
      while (count < pFD->szPage) {
        SCryptOpts opts = {0};
        opts.len = 128;
        opts.source = pFD->pBuf + count;
        opts.result = PacketData;
        opts.unitLen = 128;
        //strncpy(opts.key, tsEncryptKey, 16);
        strncpy(opts.key, encryptKey, ENCRYPT_KEY_LEN);

        NewLen = CBC_Encrypt(&opts);

        memcpy(pFD->pBuf + count, PacketData, NewLen);
        count += NewLen; 
      }
      //tsdbDebug("CBC_Encrypt count:%d %s", count, __FUNCTION__);
    }

    n = taosWriteFile(pFD->pFD, pFD->pBuf, pFD->szPage);
    if (n < 0) {
      code = TAOS_SYSTEM_ERROR(errno);
      goto _exit;
    }

    if (pFD->szFile < pFD->pgno) {
      pFD->szFile = pFD->pgno;
    }
  }
  pFD->pgno = 0;

_exit:
  return code;
}

static int32_t tsdbReadFilePage(STsdbFD *pFD, int64_t pgno, int32_t encryptAlgorithm, char* encryptKey) {
  int32_t code = 0;

  // ASSERT(pgno <= pFD->szFile);
  if (!pFD->pFD) {
    code = tsdbOpenFileImpl(pFD);
    if (code) {
      goto _exit;
    }
  }

  int64_t offset = PAGE_OFFSET(pgno, pFD->szPage);
  if (pFD->lcn > 1) {
    SVnodeCfg *pCfg = &pFD->pTsdb->pVnode->config;
    int64_t    chunksize = (int64_t)pCfg->tsdbPageSize * pCfg->s3ChunkSize;
    int64_t    chunkoffset = chunksize * (pFD->lcn - 1);

    offset -= chunkoffset;
  }
  ASSERT(offset >= 0);
  /*
  if (pFD->s3File) {
    LRUHandle *handle = NULL;

    pFD->blkno = (pgno + tsS3BlockSize - 1) / tsS3BlockSize;
    code = tsdbCacheGetBlockS3(pFD->pTsdb->bCache, pFD, &handle);
    if (code != TSDB_CODE_SUCCESS || handle == NULL) {
      tsdbCacheRelease(pFD->pTsdb->bCache, handle);
      if (code == TSDB_CODE_SUCCESS && !handle) {
        code = TSDB_CODE_OUT_OF_MEMORY;
      }
      goto _exit;
    }

    uint8_t *pBlock = (uint8_t *)taosLRUCacheValue(pFD->pTsdb->bCache, handle);

    int64_t blk_offset = (pFD->blkno - 1) * tsS3BlockSize * pFD->szPage;
    memcpy(pFD->pBuf, pBlock + (offset - blk_offset), pFD->szPage);

    tsdbCacheRelease(pFD->pTsdb->bCache, handle);
  } else {
  */
  // seek
  int64_t n = taosLSeekFile(pFD->pFD, offset, SEEK_SET);
  if (n < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _exit;
  }

<<<<<<< HEAD
    // read
    n = taosReadFile(pFD->pFD, pFD->pBuf, pFD->szPage);
    if (n < 0) {
      code = TAOS_SYSTEM_ERROR(errno);
      goto _exit;
    } else if (n < pFD->szPage) {
      code = TSDB_CODE_FILE_CORRUPTED;
      goto _exit;
    }

    if(encryptAlgorithm == DND_CA_SM4){
    //if(tsiEncryptAlgorithm == DND_CA_SM4 && (tsiEncryptScope & DND_CS_TSDB) == DND_CS_TSDB){
      unsigned char		PacketData[128];
      int		NewLen;

      int32_t count = 0;
      while(count < pFD->szPage)
      {
        SCryptOpts opts = {0};
        opts.len = 128;
        opts.source = pFD->pBuf + count;
        opts.result = PacketData;
        opts.unitLen = 128;
        //strncpy(opts.key, tsEncryptKey, 16);
        strncpy(opts.key, encryptKey, ENCRYPT_KEY_LEN);

        NewLen = CBC_Decrypt(&opts);

        memcpy(pFD->pBuf + count, PacketData, NewLen);
        count += NewLen;
      }
      //tsdbDebug("CBC_Decrypt count:%d %s", count, __FUNCTION__);
    }
=======
  // read
  n = taosReadFile(pFD->pFD, pFD->pBuf, pFD->szPage);
  if (n < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _exit;
  } else if (n < pFD->szPage) {
    code = TSDB_CODE_FILE_CORRUPTED;
    goto _exit;
>>>>>>> 3.0
  }
  //}

  // check
  if (pgno > 1 && !taosCheckChecksumWhole(pFD->pBuf, pFD->szPage)) {
    code = TSDB_CODE_FILE_CORRUPTED;
    goto _exit;
  }

  pFD->pgno = pgno;

_exit:
  return code;
}

int32_t tsdbWriteFile(STsdbFD *pFD, int64_t offset, const uint8_t *pBuf, int64_t size, int32_t encryptAlgorithm, 
                      char* encryptKey) {
  int32_t code = 0;
  int64_t fOffset = LOGIC_TO_FILE_OFFSET(offset, pFD->szPage);
  int64_t pgno = OFFSET_PGNO(fOffset, pFD->szPage);
  int64_t bOffset = fOffset % pFD->szPage;
  int64_t n = 0;

  do {
    if (pFD->pgno != pgno) {
      code = tsdbWriteFilePage(pFD, encryptAlgorithm, encryptKey);
      if (code) goto _exit;

      if (pgno <= pFD->szFile) {
        code = tsdbReadFilePage(pFD, pgno, encryptAlgorithm, encryptKey);
        if (code) goto _exit;
      } else {
        pFD->pgno = pgno;
      }
    }

    int64_t nWrite = TMIN(PAGE_CONTENT_SIZE(pFD->szPage) - bOffset, size - n);
    memcpy(pFD->pBuf + bOffset, pBuf + n, nWrite);

    pgno++;
    bOffset = 0;
    n += nWrite;
  } while (n < size);

_exit:
  return code;
}

static int32_t tsdbReadFileImp(STsdbFD *pFD, int64_t offset, uint8_t *pBuf, int64_t size, int32_t encryptAlgorithm, 
                                char* encryptKey) {
  int32_t code = 0;
  int64_t n = 0;
  int64_t fOffset = LOGIC_TO_FILE_OFFSET(offset, pFD->szPage);
  int64_t pgno = OFFSET_PGNO(fOffset, pFD->szPage);
  int32_t szPgCont = PAGE_CONTENT_SIZE(pFD->szPage);
  int64_t bOffset = fOffset % pFD->szPage;

  // ASSERT(pgno && pgno <= pFD->szFile);
  ASSERT(bOffset < szPgCont);

  while (n < size) {
    if (pFD->pgno != pgno) {
      code = tsdbReadFilePage(pFD, pgno, encryptAlgorithm, encryptKey);
      if (code) goto _exit;
    }

    int64_t nRead = TMIN(szPgCont - bOffset, size - n);
    memcpy(pBuf + n, pFD->pBuf + bOffset, nRead);

    n += nRead;
    pgno++;
    bOffset = 0;
  }

_exit:
  return code;
}

static int32_t tsdbReadFileBlock(STsdbFD *pFD, int64_t offset, int64_t size, bool check, uint8_t **ppBlock) {
  int32_t    code = 0;
  SVnodeCfg *pCfg = &pFD->pTsdb->pVnode->config;
  int64_t    chunksize = (int64_t)pCfg->tsdbPageSize * pCfg->s3ChunkSize;
  int64_t    cOffset = offset % chunksize;
  int64_t    n = 0;

  char   *object_name = taosDirEntryBaseName(pFD->path);
  char    object_name_prefix[TSDB_FILENAME_LEN];
  int32_t node_id = vnodeNodeId(pFD->pTsdb->pVnode);
  snprintf(object_name_prefix, TSDB_FQDN_LEN, "%d/%s", node_id, object_name);

  char *dot = strrchr(object_name_prefix, '.');
  if (!dot) {
    tsdbError("unexpected path: %s", object_name_prefix);
    code = TAOS_SYSTEM_ERROR(ENOENT);
    goto _exit;
  }

  char *buf = taosMemoryCalloc(1, size);

  for (int32_t chunkno = offset / chunksize + 1; n < size; ++chunkno) {
    int64_t nRead = TMIN(chunksize - cOffset, size - n);

    if (chunkno >= pFD->lcn) {
      // read last chunk
      int64_t ret = taosLSeekFile(pFD->pFD, chunksize * (chunkno - pFD->lcn) + cOffset, SEEK_SET);
      if (ret < 0) {
        code = TAOS_SYSTEM_ERROR(errno);
        taosMemoryFree(buf);
        goto _exit;
      }

      ret = taosReadFile(pFD->pFD, buf + n, nRead);
      if (ret < 0) {
        code = TAOS_SYSTEM_ERROR(errno);
        taosMemoryFree(buf);
        goto _exit;
      } else if (ret < nRead) {
        code = TSDB_CODE_FILE_CORRUPTED;
        taosMemoryFree(buf);
        goto _exit;
      }
    } else {
      uint8_t *pBlock = NULL;

      snprintf(dot + 1, TSDB_FQDN_LEN - (dot + 1 - object_name_prefix), "%d.data", chunkno);

      code = s3GetObjectBlock(object_name_prefix, cOffset, nRead, check, &pBlock);
      if (code != TSDB_CODE_SUCCESS) {
        taosMemoryFree(buf);
        goto _exit;
      }

      memcpy(buf + n, pBlock, nRead);
      taosMemoryFree(pBlock);
    }

    n += nRead;
    cOffset = 0;
  }

  *ppBlock = buf;

_exit:
  return code;
}

static int32_t tsdbReadFileS3(STsdbFD *pFD, int64_t offset, uint8_t *pBuf, int64_t size, int64_t szHint) {
  int32_t code = 0;
  int64_t n = 0;
  int32_t szPgCont = PAGE_CONTENT_SIZE(pFD->szPage);
  int64_t fOffset = LOGIC_TO_FILE_OFFSET(offset, pFD->szPage);
  int64_t pgno = OFFSET_PGNO(fOffset, pFD->szPage);
  int64_t bOffset = fOffset % pFD->szPage;

  ASSERT(bOffset < szPgCont);

  // 1, find pgnoStart & pgnoEnd to fetch from s3, if all pgs are local, no need to fetch
  // 2, fetch pgnoStart ~ pgnoEnd from s3
  // 3, store pgs to pcache & last pg to pFD->pBuf
  // 4, deliver pgs to [pBuf, pBuf + size)

  while (n < size) {
    if (pFD->pgno != pgno) {
      LRUHandle *handle = NULL;
      code = tsdbCacheGetPageS3(pFD->pTsdb->pgCache, pFD, pgno, &handle);
      if (code != TSDB_CODE_SUCCESS) {
        if (handle) {
          tsdbCacheRelease(pFD->pTsdb->pgCache, handle);
        }
        goto _exit;
      }

      if (!handle) {
        break;
      }

      uint8_t *pPage = (uint8_t *)taosLRUCacheValue(pFD->pTsdb->pgCache, handle);
      memcpy(pFD->pBuf, pPage, pFD->szPage);
      tsdbCacheRelease(pFD->pTsdb->pgCache, handle);

      // check
      if (pgno > 1 && !taosCheckChecksumWhole(pFD->pBuf, pFD->szPage)) {
        code = TSDB_CODE_FILE_CORRUPTED;
        goto _exit;
      }

      pFD->pgno = pgno;
    }

    int64_t nRead = TMIN(szPgCont - bOffset, size - n);
    memcpy(pBuf + n, pFD->pBuf + bOffset, nRead);

    n += nRead;
    ++pgno;
    bOffset = 0;
  }

  if (n < size) {
    // 2, retrieve pgs from s3
    uint8_t *pBlock = NULL;
    int64_t  retrieve_offset = PAGE_OFFSET(pgno, pFD->szPage);
    int64_t  pgnoEnd = pgno - 1 + (bOffset + size - n + szPgCont - 1) / szPgCont;

    if (szHint > 0) {
      pgnoEnd = pgno - 1 + (bOffset + szHint - n + szPgCont - 1) / szPgCont;
    }

    int64_t retrieve_size = (pgnoEnd - pgno + 1) * pFD->szPage;
    /*
    code = s3GetObjectBlock(pFD->objName, retrieve_offset, retrieve_size, 1, &pBlock);
    if (code != TSDB_CODE_SUCCESS) {
      goto _exit;
    }
    */
    code = tsdbReadFileBlock(pFD, retrieve_offset, retrieve_size, 1, &pBlock);
    if (code != TSDB_CODE_SUCCESS) {
      goto _exit;
    }
    // 3, Store Pages in Cache
    int nPage = pgnoEnd - pgno + 1;
    for (int i = 0; i < nPage; ++i) {
      if (pFD->szFile != pgno) {  // DONOT cache last volatile page
        tsdbCacheSetPageS3(pFD->pTsdb->pgCache, pFD, pgno, pBlock + i * pFD->szPage);
      }

      if (szHint > 0 && n >= size) {
        ++pgno;
        continue;
      }
      memcpy(pFD->pBuf, pBlock + i * pFD->szPage, pFD->szPage);

      // check
      if (pgno > 1 && !taosCheckChecksumWhole(pFD->pBuf, pFD->szPage)) {
        code = TSDB_CODE_FILE_CORRUPTED;
        goto _exit;
      }

      pFD->pgno = pgno;

      int64_t nRead = TMIN(szPgCont - bOffset, size - n);
      memcpy(pBuf + n, pFD->pBuf + bOffset, nRead);

      n += nRead;
      ++pgno;
      bOffset = 0;
    }

    taosMemoryFree(pBlock);
  }

_exit:
  return code;
}

int32_t tsdbReadFile(STsdbFD *pFD, int64_t offset, uint8_t *pBuf, int64_t size, int64_t szHint, 
                    int32_t encryptAlgorithm, char* encryptKey) {
  int32_t code = 0;
  if (!pFD->pFD) {
    code = tsdbOpenFileImpl(pFD);
    if (code) {
      goto _exit;
    }
  }

  if (pFD->lcn > 1 /*pFD->s3File && tsS3BlockSize < 0*/) {
    return tsdbReadFileS3(pFD, offset, pBuf, size, szHint);
  } else {
    return tsdbReadFileImp(pFD, offset, pBuf, size, encryptAlgorithm, encryptKey);
  }

_exit:
  return code;
}

int32_t tsdbReadFileToBuffer(STsdbFD *pFD, int64_t offset, int64_t size, SBuffer *buffer, int64_t szHint,
                            int32_t encryptAlgorithm, char* encryptKey) {
  int32_t code;

  code = tBufferEnsureCapacity(buffer, buffer->size + size);
  if (code) return code;
  code = tsdbReadFile(pFD, offset, (uint8_t *)tBufferGetDataEnd(buffer), size, szHint,
                      encryptAlgorithm, encryptKey);
  if (code) return code;
  buffer->size += size;

  return code;
}

int32_t tsdbFsyncFile(STsdbFD *pFD, int32_t encryptAlgorithm, char* encryptKey) {
  int32_t code = 0;
  /*
  if (pFD->s3File) {
    tsdbWarn("%s file: %s", __func__, pFD->path);
    return code;
  }
<<<<<<< HEAD
  code = tsdbWriteFilePage(pFD, encryptAlgorithm, encryptKey);
=======
  */
  code = tsdbWriteFilePage(pFD);
>>>>>>> 3.0
  if (code) goto _exit;

  if (taosFsyncFile(pFD->pFD) < 0) {
    code = TAOS_SYSTEM_ERROR(errno);
    goto _exit;
  }

_exit:
  return code;
}

// SDataFReader ====================================================
int32_t tsdbDataFReaderOpen(SDataFReader **ppReader, STsdb *pTsdb, SDFileSet *pSet) {
  int32_t       code = 0;
  int32_t       lino = 0;
  SDataFReader *pReader = NULL;
  int32_t       szPage = pTsdb->pVnode->config.tsdbPageSize;
  char          fname[TSDB_FILENAME_LEN];

  // alloc
  pReader = (SDataFReader *)taosMemoryCalloc(1, sizeof(*pReader));
  if (pReader == NULL) {
    code = TSDB_CODE_OUT_OF_MEMORY;
    TSDB_CHECK_CODE(code, lino, _exit);
  }
  pReader->pTsdb = pTsdb;
  pReader->pSet = pSet;

  // head
  tsdbHeadFileName(pTsdb, pSet->diskId, pSet->fid, pSet->pHeadF, fname);
  code = tsdbOpenFile(fname, pTsdb, TD_FILE_READ, &pReader->pHeadFD, 0);
  TSDB_CHECK_CODE(code, lino, _exit);

  // data
  tsdbDataFileName(pTsdb, pSet->diskId, pSet->fid, pSet->pDataF, fname);
  code = tsdbOpenFile(fname, pTsdb, TD_FILE_READ, &pReader->pDataFD, 0);
  TSDB_CHECK_CODE(code, lino, _exit);

  // sma
  tsdbSmaFileName(pTsdb, pSet->diskId, pSet->fid, pSet->pSmaF, fname);
  code = tsdbOpenFile(fname, pTsdb, TD_FILE_READ, &pReader->pSmaFD, 0);
  TSDB_CHECK_CODE(code, lino, _exit);

  // stt
  for (int32_t iStt = 0; iStt < pSet->nSttF; iStt++) {
    tsdbSttFileName(pTsdb, pSet->diskId, pSet->fid, pSet->aSttF[iStt], fname);
    code = tsdbOpenFile(fname, pTsdb, TD_FILE_READ, &pReader->aSttFD[iStt], 0);
    TSDB_CHECK_CODE(code, lino, _exit);
  }

_exit:
  if (code) {
    *ppReader = NULL;
    tsdbError("vgId:%d, %s failed at line %d since %s", TD_VID(pTsdb->pVnode), __func__, lino, tstrerror(code));

    if (pReader) {
      for (int32_t iStt = 0; iStt < pSet->nSttF; iStt++) tsdbCloseFile(&pReader->aSttFD[iStt]);
      tsdbCloseFile(&pReader->pSmaFD);
      tsdbCloseFile(&pReader->pDataFD);
      tsdbCloseFile(&pReader->pHeadFD);
      taosMemoryFree(pReader);
    }
  } else {
    *ppReader = pReader;
  }
  return code;
}

int32_t tsdbDataFReaderClose(SDataFReader **ppReader) {
  int32_t code = 0;
  if (*ppReader == NULL) return code;

  // head
  tsdbCloseFile(&(*ppReader)->pHeadFD);

  // data
  tsdbCloseFile(&(*ppReader)->pDataFD);

  // sma
  tsdbCloseFile(&(*ppReader)->pSmaFD);

  // stt
  for (int32_t iStt = 0; iStt < TSDB_STT_TRIGGER_ARRAY_SIZE; iStt++) {
    if ((*ppReader)->aSttFD[iStt]) {
      tsdbCloseFile(&(*ppReader)->aSttFD[iStt]);
    }
  }

  for (int32_t iBuf = 0; iBuf < sizeof((*ppReader)->aBuf) / sizeof(uint8_t *); iBuf++) {
    tFree((*ppReader)->aBuf[iBuf]);
  }
  taosMemoryFree(*ppReader);
  *ppReader = NULL;
  return code;
}

int32_t tsdbReadBlockIdx(SDataFReader *pReader, SArray *aBlockIdx) {
  int32_t    code = 0;
  SHeadFile *pHeadFile = pReader->pSet->pHeadF;
  int64_t    offset = pHeadFile->offset;
  int64_t    size = pHeadFile->size - offset;

  taosArrayClear(aBlockIdx);
  if (size == 0) return code;

  // alloc
  code = tRealloc(&pReader->aBuf[0], size);
  if (code) goto _err;

  // read
  int32_t encryptAlgorithm = pReader->pTsdb->pVnode->config.tsdbCfg.encryptAlgorithm;
  char* encryptKey = pReader->pTsdb->pVnode->config.tsdbCfg.encryptKey;
  code = tsdbReadFile(pReader->pHeadFD, offset, pReader->aBuf[0], size, 0, encryptAlgorithm, encryptKey);
  if (code) goto _err;

  // decode
  int64_t n = 0;
  while (n < size) {
    SBlockIdx blockIdx;
    n += tGetBlockIdx(pReader->aBuf[0] + n, &blockIdx);

    if (taosArrayPush(aBlockIdx, &blockIdx) == NULL) {
      code = TSDB_CODE_OUT_OF_MEMORY;
      goto _err;
    }
  }
  ASSERT(n == size);

  return code;

_err:
  tsdbError("vgId:%d, read block idx failed since %s", TD_VID(pReader->pTsdb->pVnode), tstrerror(code));
  return code;
}

int32_t tsdbReadSttBlk(SDataFReader *pReader, int32_t iStt, SArray *aSttBlk) {
  int32_t   code = 0;
  SSttFile *pSttFile = pReader->pSet->aSttF[iStt];
  int64_t   offset = pSttFile->offset;
  int64_t   size = pSttFile->size - offset;

  taosArrayClear(aSttBlk);
  if (size == 0) return code;

  // alloc
  code = tRealloc(&pReader->aBuf[0], size);
  if (code) goto _err;

  // read
  int32_t encryptAlgorithm = pReader->pTsdb->pVnode->config.tsdbCfg.encryptAlgorithm;
  char* encryptKey = pReader->pTsdb->pVnode->config.tsdbCfg.encryptKey;
  code = tsdbReadFile(pReader->aSttFD[iStt], offset, pReader->aBuf[0], size, 0, encryptAlgorithm, encryptKey);
  if (code) goto _err;

  // decode
  int64_t n = 0;
  while (n < size) {
    SSttBlk sttBlk;
    n += tGetSttBlk(pReader->aBuf[0] + n, &sttBlk);

    if (taosArrayPush(aSttBlk, &sttBlk) == NULL) {
      code = TSDB_CODE_OUT_OF_MEMORY;
      goto _err;
    }
  }
  ASSERT(n == size);

  return code;

_err:
  tsdbError("vgId:%d, read stt blk failed since %s", TD_VID(pReader->pTsdb->pVnode), tstrerror(code));
  return code;
}

int32_t tsdbReadDataBlk(SDataFReader *pReader, SBlockIdx *pBlockIdx, SMapData *mDataBlk) {
  int32_t code = 0;
  int64_t offset = pBlockIdx->offset;
  int64_t size = pBlockIdx->size;

  // alloc
  code = tRealloc(&pReader->aBuf[0], size);
  if (code) goto _err;

  // read
  int32_t encryptAlgorithm = pReader->pTsdb->pVnode->config.tsdbCfg.encryptAlgorithm;
  char* encryptKey = pReader->pTsdb->pVnode->config.tsdbCfg.encryptKey;
  code = tsdbReadFile(pReader->pHeadFD, offset, pReader->aBuf[0], size, 0, encryptAlgorithm, encryptKey);
  if (code) goto _err;

  // decode
  int64_t n = tGetMapData(pReader->aBuf[0], mDataBlk);
  if (n < 0) {
    code = TSDB_CODE_OUT_OF_MEMORY;
    goto _err;
  }
  ASSERT(n == size);

  return code;

_err:
  tsdbError("vgId:%d, read block failed since %s", TD_VID(pReader->pTsdb->pVnode), tstrerror(code));
  return code;
}

// SDelFReader ====================================================
struct SDelFReader {
  STsdb   *pTsdb;
  SDelFile fDel;
  STsdbFD *pReadH;
  uint8_t *aBuf[1];
};

int32_t tsdbDelFReaderOpen(SDelFReader **ppReader, SDelFile *pFile, STsdb *pTsdb) {
  int32_t      code = 0;
  int32_t      lino = 0;
  char         fname[TSDB_FILENAME_LEN];
  SDelFReader *pDelFReader = NULL;

  // alloc
  pDelFReader = (SDelFReader *)taosMemoryCalloc(1, sizeof(*pDelFReader));
  if (pDelFReader == NULL) {
    code = TSDB_CODE_OUT_OF_MEMORY;
    goto _exit;
  }

  // open impl
  pDelFReader->pTsdb = pTsdb;
  pDelFReader->fDel = *pFile;

  tsdbDelFileName(pTsdb, pFile, fname);
  code = tsdbOpenFile(fname, pTsdb, TD_FILE_READ, &pDelFReader->pReadH, 0);
  if (code) {
    taosMemoryFree(pDelFReader);
    goto _exit;
  }

_exit:
  if (code) {
    *ppReader = NULL;
    tsdbError("vgId:%d, %s failed at %d since %s", TD_VID(pTsdb->pVnode), __func__, lino, tstrerror(code));
  } else {
    *ppReader = pDelFReader;
  }
  return code;
}

int32_t tsdbDelFReaderClose(SDelFReader **ppReader) {
  int32_t      code = 0;
  SDelFReader *pReader = *ppReader;

  if (pReader) {
    tsdbCloseFile(&pReader->pReadH);
    for (int32_t iBuf = 0; iBuf < sizeof(pReader->aBuf) / sizeof(uint8_t *); iBuf++) {
      tFree(pReader->aBuf[iBuf]);
    }
    taosMemoryFree(pReader);
  }

  *ppReader = NULL;
  return code;
}

int32_t tsdbReadDelData(SDelFReader *pReader, SDelIdx *pDelIdx, SArray *aDelData) {
  return tsdbReadDelDatav1(pReader, pDelIdx, aDelData, INT64_MAX);
}

int32_t tsdbReadDelDatav1(SDelFReader *pReader, SDelIdx *pDelIdx, SArray *aDelData, int64_t maxVer) {
  int32_t code = 0;
  int64_t offset = pDelIdx->offset;
  int64_t size = pDelIdx->size;
  int64_t n;

  taosArrayClear(aDelData);

  // alloc
  code = tRealloc(&pReader->aBuf[0], size);
  if (code) goto _err;

  // read
  int32_t encryptAlgorithm = pReader->pTsdb->pVnode->config.tsdbCfg.encryptAlgorithm;
  char* encryptKey = pReader->pTsdb->pVnode->config.tsdbCfg.encryptKey;
  code = tsdbReadFile(pReader->pReadH, offset, pReader->aBuf[0], size, 0, encryptAlgorithm, encryptKey);
  if (code) goto _err;

  // // decode
  n = 0;
  while (n < size) {
    SDelData delData;
    n += tGetDelData(pReader->aBuf[0] + n, &delData);

    if (delData.version > maxVer) {
      continue;
    }
    if (taosArrayPush(aDelData, &delData) == NULL) {
      code = TSDB_CODE_OUT_OF_MEMORY;
      goto _err;
    }
  }

  ASSERT(n == size);

  return code;

_err:
  tsdbError("vgId:%d, read del data failed since %s", TD_VID(pReader->pTsdb->pVnode), tstrerror(code));
  return code;
}

int32_t tsdbReadDelIdx(SDelFReader *pReader, SArray *aDelIdx) {
  int32_t code = 0;
  int32_t n;
  int64_t offset = pReader->fDel.offset;
  int64_t size = pReader->fDel.size - offset;

  taosArrayClear(aDelIdx);

  // alloc
  code = tRealloc(&pReader->aBuf[0], size);
  if (code) goto _err;

  // read
  int32_t encryptAlgorithm = pReader->pTsdb->pVnode->config.tsdbCfg.encryptAlgorithm;
  char* encryptKey = pReader->pTsdb->pVnode->config.tsdbCfg.encryptKey;
  code = tsdbReadFile(pReader->pReadH, offset, pReader->aBuf[0], size, 0, encryptAlgorithm, encryptKey);
  if (code) goto _err;

  // decode
  n = 0;
  while (n < size) {
    SDelIdx delIdx;

    n += tGetDelIdx(pReader->aBuf[0] + n, &delIdx);

    if (taosArrayPush(aDelIdx, &delIdx) == NULL) {
      code = TSDB_CODE_OUT_OF_MEMORY;
      goto _err;
    }
  }

  ASSERT(n == size);

  return code;

_err:
  tsdbError("vgId:%d, read del idx failed since %s", TD_VID(pReader->pTsdb->pVnode), tstrerror(code));
  return code;
}
