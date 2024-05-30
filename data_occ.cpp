#include "data_occ.h"

data_occ::data_occ() {
	his_len = 0;
	active_len = 0;
	history = NULL;
	active = NULL;
}

/*
 * 1. 获取结束时间戳end_ts，以及历史提交列表history，当前活跃事务列表active，并把当前事务加入active
 * 2. 对当前事务的读写集进行验证，判断是否有冲突
 *      2.1 验证当前事务的读集合是否与history中事务的写集合有重叠
 *      2.2 验证当前事务的读集合和写集合是否与active中事务的读集合有重叠
 * 3. 基于2中的验证结果，有以下两种情况
 *      3.1 若有冲突，则abort掉当前事务，并从active中移出
 *      3.2 若无冲突，则将当前事务的写操作更新到数据库中
 * 4. 将当前事务从active中移出并加入history
 */
RC data_occ::validate(txn_man * txn) {
  RC rc = RCOK;
  txn->end_ts = time(0);
  time_t start_ts = txn->start_ts;
  time_t end_ts = txn->end_ts;

  setEntry *rset = new setEntry();
  setEntry *wset = new setEntry();
  get_rw_set(txn,rset,wset);

  setEntry *historyEntry = history;
  setEntry *activeEntry = active;

  latch.lock();
  if(wset->set_size!=0){
    active_len++;
    wset->next = active;
    active = wset; // 将当前的事务添加再active的头部
  }
  latch.unlock();

  while (historyEntry!=NULL)
  {
    if(is_overlap(historyEntry,start_ts,end_ts)){
      if(conflict(historyEntry,rset)){// 已完成的事务的写集与当前事务的读集有冲突
        if(wset->set_size!=0){
          latch.lock();
          activeEntry=active;
          setEntry *prev = NULL;
          while(activeEntry->txn!=txn){
            prev = activeEntry;
            activeEntry = activeEntry->next;
          }
          if(prev!=NULL){
            prev->next = activeEntry->next;
          }else{
            active = activeEntry->next;
          }
          active_len--;
          latch.unlock();
        }
        rc = ABORT;
        return rc;
      }
    }
    historyEntry = historyEntry->next;
  }
  while (activeEntry!=NULL)
  {
    if (conflict(rset,activeEntry)||conflict(wset,activeEntry))
    {
      if(wset->set_size!=0){
        latch.lock();
        activeEntry=active;
        setEntry *prev = NULL;
        while(activeEntry->txn!=txn){
          prev = activeEntry;
          activeEntry = activeEntry->next;
        }
        if(prev!=NULL){
          prev->next = activeEntry->next;
        }else{
          active = activeEntry->next;
        }
        active_len--;
        latch.unlock();
      }
      rc=ABORT;
      return rc;
    }
    activeEntry=activeEntry->next;
  }
  // 从当前active中移出事务并添加值history中
  latch.lock();
  activeEntry=active;
  setEntry *prev = NULL;
  while(activeEntry->txn!=txn){
    prev = activeEntry;
    activeEntry = activeEntry->next;
  }
  if(prev!=NULL){
    prev->next = activeEntry->next;
  }else{
    active = activeEntry->next;
  }
  active_len--;
  time_t ts = time(0);
  txn->commit_ts=ts;
  wset->next=history;
  history=wset;
  his_len++;
  for (int i = 0; i < wset->set_size; i++)
  {
    wset->datalist[i]->value = wset->values[i];
  }
  latch.unlock();

  return rc;
}

/*
 * 验证两个集合是否有冲突
 */
bool data_occ::conflict(setEntry * set1, setEntry * set2) {
  if(set1->txn->tid==set2->txn->tid){
    return false;
  }
  for (int i = 0; i < set1->set_size; i++)
  {
    for (int j = 0; j < set2->set_size; j++)
    {
      if(set1->datalist[i]==set2->datalist[j]){
        return true;
      }
    }
  }

  return false;
}

/*
 * 验证两个事务是否时间上有重合，参考ppt p7中关于时间的判定条件
 */
bool data_occ::is_overlap(setEntry * set, time_t start, time_t end) {
  if (set->txn->commit_ts<=start)
  {
    return false;
  }else if(set->txn->commit_ts<=end){
    return true;
  }else return false;
}

/*
 * 获取当前事务的读写集
 */
RC data_occ::get_rw_set(txn_man * txn, setEntry * &rset, setEntry * &wset) {
	wset->set_size = txn->wr_cnt;
	rset->set_size = txn->rd_cnt;
	wset->txn = txn;
	rset->txn = txn;
	for (int i = 0; i < txn->wr_cnt; i++) {
		wset->datalist.push_back(txn->wr_list[i]->data);
		wset->values.push_back(txn->wr_list[i]->value);
	}
	for (int j = 0; j < txn->rd_cnt; j++) {
		rset->datalist.push_back(txn->rd_list[j]);
	}
	return RCOK;
}
