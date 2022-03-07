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

#ifndef _TD_TSDB_SMA_H_
#define _TD_TSDB_SMA_H_

// insert/update interface
int32_t tsdbInsertTSmaDataImpl(STsdb *pTsdb, STSma *param, STSmaData *pData);
int32_t tsdbInsertRSmaDataImpl(STsdb *pTsdb, SRSma *param, STSmaData *pData);


// query interface
// TODO: This is the basic params, and should wrap the params to a queryHandle.
int32_t tsdbGetTSmaDataImpl(STsdb *pTsdb, STSma *param, STSmaData *pData, STimeWindow *queryWin, int32_t nMaxResult);

// management interface
int32_t tsdbGetTSmaStatus(STsdb *pTsdb, STSma *param, void* result);
int32_t tsdbRemoveTSmaData(STsdb *pTsdb, STSma *param, STimeWindow *pWin);




// internal func
static FORCE_INLINE int32_t tsdbEncodeTSmaKey(uint64_t tableUid, col_id_t colId, TSKEY tsKey, void **pData) {
  int32_t len = 0;
  len += taosEncodeFixedU64(pData, tableUid);
  len += taosEncodeFixedU16(pData, colId);
  len += taosEncodeFixedI64(pData, tsKey);
  return len;
}

#endif /* _TD_TSDB_SMA_H_ */