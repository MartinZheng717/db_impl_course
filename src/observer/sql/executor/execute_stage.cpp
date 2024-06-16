/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/4/13.
//

#include <string>
#include <sstream>

#include "execute_stage.h"

#include "common/io/io.h"
#include "common/log/log.h"
#include "common/seda/timer_stage.h"
#include "common/lang/string.h"
#include "session/session.h"
#include "event/storage_event.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "event/execution_plan_event.h"
#include "sql/executor/execution_node.h"
#include "sql/executor/tuple.h"
#include "storage/common/table.h"
#include "storage/default/default_handler.h"
#include "storage/common/condition_filter.h"
#include "storage/trx/trx.h"

using namespace common;

RC create_selection_executor(
        Trx *trx, const Selects &selects, const char *db, const char *table_name, SelectExeNode &select_node);

//! Constructor
ExecuteStage::ExecuteStage(const char *tag) : Stage(tag)
{}

//! Destructor
ExecuteStage::~ExecuteStage()
{}

//! Parse properties, instantiate a stage object
Stage *ExecuteStage::make_stage(const std::string &tag)
{
  ExecuteStage *stage = new (std::nothrow) ExecuteStage(tag.c_str());
  if (stage == nullptr) {
    LOG_ERROR("new ExecuteStage failed");
    return nullptr;
  }
  stage->set_properties();
  return stage;
}

//! Set properties for this object set in stage specific properties
bool ExecuteStage::set_properties()
{
  //  std::string stageNameStr(stageName);
  //  std::map<std::string, std::string> section = theGlobalProperties()->get(
  //    stageNameStr);
  //
  //  std::map<std::string, std::string>::iterator it;
  //
  //  std::string key;

  return true;
}

//! Initialize stage params and validate outputs
bool ExecuteStage::initialize()
{
  LOG_TRACE("Enter");

  std::list<Stage *>::iterator stgp = next_stage_list_.begin();
  default_storage_stage_ = *(stgp++);
  mem_storage_stage_ = *(stgp++);

  LOG_TRACE("Exit");
  return true;
}

//! Cleanup after disconnection
void ExecuteStage::cleanup()
{
  LOG_TRACE("Enter");

  LOG_TRACE("Exit");
}

void ExecuteStage::handle_event(StageEvent *event)
{
  LOG_TRACE("Enter\n");

  handle_request(event);

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::callback_event(StageEvent *event, CallbackContext *context)
{
  LOG_TRACE("Enter\n");

  // here finish read all data from disk or network, but do nothing here.
  ExecutionPlanEvent *exe_event = static_cast<ExecutionPlanEvent *>(event);
  SQLStageEvent *sql_event = exe_event->sql_event();
  sql_event->done_immediate();

  LOG_TRACE("Exit\n");
  return;
}

void ExecuteStage::handle_request(common::StageEvent *event)
{
  ExecutionPlanEvent *exe_event = static_cast<ExecutionPlanEvent *>(event);
  SessionEvent *session_event = exe_event->sql_event()->session_event();
  Query *sql = exe_event->sqls();
  const char *current_db = session_event->get_client()->session->get_current_db().c_str();

  CompletionCallback *cb = new (std::nothrow) CompletionCallback(this, nullptr);
  if (cb == nullptr) {
    LOG_ERROR("Failed to new callback for ExecutionPlanEvent");
    exe_event->done_immediate();
    return;
  }
  exe_event->push_callback(cb);

  switch (sql->flag) {
    case SCF_SELECT: {  // select
      do_select(current_db, sql, exe_event->sql_event()->session_event());
      exe_event->done_immediate();
    } break;

    case SCF_INSERT:
    case SCF_UPDATE:
    case SCF_DELETE:
    case SCF_CREATE_TABLE:
    case SCF_SHOW_TABLES:
    case SCF_DESC_TABLE:
    case SCF_DROP_TABLE:
    case SCF_CREATE_INDEX:
    case SCF_DROP_INDEX:
    case SCF_LOAD_DATA: {
      StorageEvent *storage_event = new (std::nothrow) StorageEvent(exe_event);
      if (storage_event == nullptr) {
        LOG_ERROR("Failed to new StorageEvent");
        event->done_immediate();
        return;
      }

      default_storage_stage_->handle_event(storage_event);
    } break;
    case SCF_SYNC: {
      RC rc = DefaultHandler::get_default().sync();
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    } break;
    case SCF_BEGIN: {
      session_event->get_client()->session->set_trx_multi_operation_mode(true);
      session_event->set_response(strrc(RC::SUCCESS));
      exe_event->done_immediate();
    } break;
    case SCF_COMMIT: {
      Trx *trx = session_event->get_client()->session->current_trx();
      RC rc = trx->commit();
      session_event->get_client()->session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    } break;
    case SCF_ROLLBACK: {
      Trx *trx = session_event->get_client()->session->current_trx();
      RC rc = trx->rollback();
      session_event->get_client()->session->set_trx_multi_operation_mode(false);
      session_event->set_response(strrc(rc));
      exe_event->done_immediate();
    } break;
    case SCF_HELP: {
      const char *response = "show tables;\n"
                             "desc `table name`;\n"
                             "create table `table name` (`column name` `column type`, ...);\n"
                             "create index `index name` on `table` (`column`);\n"
                             "insert into `table` values(`value1`,`value2`);\n"
                             "update `table` set column=value [where `column`=`value`];\n"
                             "delete from `table` [where `column`=`value`];\n"
                             "select [ * | `columns` ] from `table`;\n";
      session_event->set_response(response);
      exe_event->done_immediate();
    } break;
    case SCF_EXIT: {
      // do nothing
      const char *response = "Unsupported\n";
      session_event->set_response(response);
      exe_event->done_immediate();
    } break;
    default: {
      exe_event->done_immediate();
      LOG_ERROR("Unsupported command=%d\n", sql->flag);
    }
  }
}

void end_trx_if_need(Session *session, Trx *trx, bool all_right)
{
  if (!session->is_trx_multi_operation_mode()) {
    if (all_right) {
      trx->commit();
    } else {
      trx->rollback();
    }
  }
}

// 需要满足多表联查条件
bool match_join_condition(const Tuple *res_tuple,
                          const std::vector<std::vector<int>> condition_idxs) {
  // res_tuple 是 需要进行筛选的某一行
  // condition_idxs 是 C x 3 数组
  // 每一条的3个元素代表（左值的属性在新schema的下标，CompOp运算符，右值的属性在新schema的下标）
  //TODO 判断表中某一行 res_tuple 是否满足多表联查条件即：左值=右值
  bool fit = true;
  for (int i = 0; i < condition_idxs.size(); i++)
  {
    std::vector<int> condition_idx = condition_idxs[i];
    int res = (res_tuple->get(condition_idx[0]).compare(res_tuple->get(condition_idx[2])));
    switch (condition_idx[1]){
      case 0:
        fit = (res==0);
        break;
      case 1:
        fit = (res<=0);
        break;
      case 2:
        fit = (res!=0);
        break;
      case 3:
        fit = (res<0);
        break;
      case 4:
        fit = (res>=0);
        break;
      case 5:
        fit = (res>0);
        break;
    }
    if(!fit){
      return false;
    }
  }
  return true;
}

// 将多段小元组合成一个大元组
Tuple merge_tuples(
        const std::vector<std::vector<Tuple>::const_iterator> temp_tuples,
        std::vector<int> orders) {
  std::vector<std::shared_ptr<TupleValue>> temp_res;
  Tuple res_tuple;
  // 原本错误的思路
  // temp_tuples [[tuple,tuple,...,tuple],[tuple,tuple,...,tuple]...]
  // tuple[1,1] 每个元组
  //eg:
  //select * from t1,t2;
  // t1.id | t1.sex
  //   1   |   1
  // t2.id | t2.age
  //   1   |   10
  //   2   |   15
  // temp_tuples_sets [[[1,1],[1,10]]],[[[1,1],[2,15]]]
  //                  tuple11 tuple21  tuple11 tuple22
  //                   temp_tuples      temp_tuples



  //TODO 先把每个字段都放到对应的位置上(temp_res)
  int table_count = temp_tuples.size()/2;
  // 不考虑n个table的选择，先只考虑2，3个table的笛卡尔积
  std::vector<std::shared_ptr<TupleValue>> temp_values;
  for (int i = 0; i < temp_tuples.size(); i++)
  {
    temp_values.clear();
    temp_values = temp_tuples[i]->values();
    for (int k = 0; k < temp_values.size(); k++)
    {
      temp_res.push_back(temp_values[k]);
    }
  }
  // std::cout<<temp_res.size()<<"\n";
  // for (int j = 0; j < temp_tuples.size(); j++)
  // {
  //   Tuple temp_tuple = temp_tuples[j];
  //   std::vector<std::shared_ptr<TupleValue>> temp_values = temp_tuple.values();
  //   for (int k = 0; k < temp_values.size(); k++)
  //   {
  //     temp_res.push_back(temp_values[k]);
  //   }
  // }
  //TODO 再依次(orders)添加到大元组(res_tuple)里即可
  for (int i = 0; i < orders.size(); i++)
  {
    for (int j = 0; j < orders.size(); j++)
    {
      if(orders[j]==i){
        res_tuple.add(temp_res[j]);
      }
    }
  }
  return res_tuple;
}

int partition_tuple(TupleSet *res_tuples, int l, int r, OrderType order_type, int order_col_idx){
  int i = l-1;
  const Tuple* pivot = &(res_tuples->get(r));
  for(int j = l; j<r; j++){

    if(pivot->get(order_col_idx).compare((&(res_tuples->get(j)))->get(order_col_idx))>0){
      if(order_type==kOrderAsc){
        i++;
        res_tuples->swap_tuple(i,j);
      }
    }else if(pivot->get(order_col_idx).compare((&(res_tuples->get(j)))->get(order_col_idx))<0){
      if(order_type==kOrderDesc){
        i++;
        res_tuples->swap_tuple(i,j);
      }
    }
  }
  res_tuples->swap_tuple(i+1,r);
  return i+1;
}

void quick_sort_tuple(TupleSet *res_tuples, int l, int r, OrderType order_type, int order_col_idx){
  if(l<r){
    int mid = partition_tuple(res_tuples, l, r, order_type, order_col_idx);
    quick_sort_tuple(res_tuples, l , mid-1,order_type, order_col_idx);
    quick_sort_tuple(res_tuples, mid+1, r, order_type, order_col_idx);
  }
}

void order_by_exec(const Selects &selects, TupleSet *res_tuples){
  if(selects.order_num>0){
    if(selects.order_num==1){
      // 排序方法（顺序和逆序）
      OrderType order_type = selects.order_des[0].type;
      char *relation_name = selects.order_des[0].relation_name;
      char *attribute_name = selects.order_des[0].attribute_name;
      if(relation_name==nullptr){
        relation_name =selects.relations[0];
      }
      // 对应排序数值在元组中的idx位置
      // if(selects.relation_num==1){
      int order_col_idx = -1;
      const TupleSchema &tupleschema = res_tuples->get_schema();
      order_col_idx = tupleschema.index_of_field(relation_name,attribute_name);
      quick_sort_tuple(res_tuples,0,res_tuples->size()-1,order_type,order_col_idx);
      // }else{

      // }
      // 快排
    }else if(selects.order_num==2){
      // 按照两个参数进行排序需要先按照后一个排序，在按照前一个排序
      OrderType order_type = selects.order_des[1].type;
      char *relation_name = selects.order_des[1].relation_name;
      char *attribute_name = selects.order_des[1].attribute_name;
      if(relation_name==nullptr){
        relation_name =selects.relations[0];
      }
      // if(selects.relation_num==1){
      // 对应排序数值在元组中的idx位置
      int order_col_idx = -1;
      const TupleSchema &tupleschema = res_tuples->get_schema();
      order_col_idx = tupleschema.index_of_field(relation_name,attribute_name);

      // 快排
      quick_sort_tuple(res_tuples,0,res_tuples->size()-1,order_type,order_col_idx);

      order_type = selects.order_des[0].type;
      relation_name = selects.order_des[0].relation_name;
      attribute_name = selects.order_des[0].attribute_name;
      // if(relation_name==nullptr){
      //   relation_name =selects.relations[0];
      // }
      // 对应排序数值在元组中的idx位置
      const TupleSchema &tupleschema2 = res_tuples->get_schema();
      order_col_idx = tupleschema2.index_of_field(relation_name,attribute_name);

      // 快排
      quick_sort_tuple(res_tuples,0,res_tuples->size()-1,order_type,order_col_idx);
      // }
    }
  }
  return;
}

// 这里没有对输入的某些信息做合法性校验，比如查询的列名、where条件中的列名等，没有做必要的合法性校验
// 需要补充上这一部分. 校验部分也可以放在resolve，不过跟execution放一起也没有关系
RC ExecuteStage::do_select(const char *db, Query *sql, SessionEvent *session_event)
{

  RC rc = RC::SUCCESS;
  Session *session = session_event->get_client()->session;
  Trx *trx = session->current_trx();
  const Selects &selects = sql->sstr.selection;
  // 把所有的表和只跟这张表关联的condition都拿出来，生成最底层的select 执行节点
  std::vector<SelectExeNode *> select_nodes;
  for (size_t i = 0; i < selects.relation_num; i++) {
    const char *table_name = selects.relations[i];
    SelectExeNode *select_node = new SelectExeNode;
    rc = create_selection_executor(trx, selects, db, table_name, *select_node);
    if (rc != RC::SUCCESS) {
      delete select_node;
      for (SelectExeNode *&tmp_node : select_nodes) {
        delete tmp_node;
      }
      end_trx_if_need(session, trx, false);
      return rc;
    }
    select_nodes.push_back(select_node);
  }

  if (select_nodes.empty()) {
    LOG_ERROR("No table given");
    end_trx_if_need(session, trx, false);
    return RC::SQL_SYNTAX;
  }

  std::vector<TupleSet> tuple_sets;
  for (SelectExeNode *&node : select_nodes) {
    TupleSet tuple_set;
    rc = node->execute(tuple_set);
    if (rc != RC::SUCCESS) {
      for (SelectExeNode *&tmp_node : select_nodes) {
        delete tmp_node;
      }
      end_trx_if_need(session, trx, false);
      return rc;
    } else {
      tuple_sets.push_back(std::move(tuple_set));
    }
  }

  std::stringstream ss;
  TupleSet print_tuples;
  if (tuple_sets.size() > 1) {
    // 本次查询了多张表，需要做join操作
    TupleSchema join_schema;
    TupleSchema old_schema;
    for (std::vector<TupleSet>::const_reverse_iterator
                 rit = tuple_sets.rbegin(),
                 rend = tuple_sets.rend();
         rit != rend; ++rit) {
      // 这里是某张表投影完的所有字段，如果是select * from t1,t2;
      // old_schema=[t1.a, t1.b, t2.a, t2.b]
      old_schema.append(rit->get_schema());
    }
    // 经过debug发现selects此时select中的attributes的顺序刚好是old_schema中的倒叙

    std::vector<int> select_order;
    //TODO 根据列名输出顺序，添加 old_schema 对应字段到 join_schema 中，并构建select_order数组
    // 如果是select * ，添加所有字段
    // 如果是select t1.*，表名匹配的加入字段
    // 如果是select t1.age，表名+字段名匹配的加入字段
    for(int i = 0 ;i<selects.attr_num;i++){
      const RelAttr &attr = selects.attributes[i];
      // 每个attr中的relation-name可以为0x0或者某个表
      // 每个attr中的attribute-name可以为*或者某个表的类名
      // attr中的relation-name和old—schema中的fields_中的table_name对应
      // attr中的attribute-name和old—schema中的fields_中的field_name对应
      if(attr.attribute_name!=nullptr){
        if(strcmp("*",attr.attribute_name)==0){
          if(attr.relation_name==nullptr){ // select *
            join_schema = old_schema;
            std::vector<TupleField> fields = old_schema.fields();

            for(int j = 0;j<fields.size();j++){
              select_order.push_back(j);
            }
          }else{ // select table.*
            std::vector<TupleField> fields = old_schema.fields();
            for(int j = 0;j<fields.size();j++){
              TupleField otherfield = fields[j];
              if(strcmp(otherfield.table_name(),attr.relation_name)==0){
                join_schema.add(otherfield);
                select_order.push_back(j);
              }
            }
          }
        }else{ // select table.coloum
          // 获取在old_schema中对应field所在order
          int field_order = old_schema.index_of_field(attr.relation_name,attr.attribute_name);
          select_order.push_back(field_order);
          // 利用order得到该field
          TupleField otherfield = old_schema.field(field_order);
          join_schema.add(otherfield);
        }
      }
    }

    print_tuples.set_schema(join_schema);

    // 构建联查的conditions需要找到对应的表
    // C x 3 数组
    // 每一条的3个元素代表（左值的属性在新schema的下标，CompOp运算符，右值的属性在新schema的下标）
    std::vector<std::vector<int>> condition_idxs;
    for (size_t i = 0; i < selects.condition_num; i++) {
      const Condition &condition = selects.conditions[i];
      if (condition.left_is_attr == 1 &&
          condition.right_is_attr == 1) {
        std::vector<int> temp_con;
        const char *l_table_name = condition.left_attr.relation_name;
        const char *l_field_name = condition.left_attr.attribute_name;
        const CompOp comp = condition.comp;
        const char *r_table_name = condition.right_attr.relation_name;
        const char *r_field_name = condition.right_attr.attribute_name;
        // 利用selects.condition设置左值右值，再依据前一步中print_tuples的设置找到对应的schema码
        temp_con.push_back(print_tuples.get_schema().index_of_field(
                l_table_name, l_field_name));
        temp_con.push_back(comp);
        temp_con.push_back(print_tuples.get_schema().index_of_field(
                r_table_name, r_field_name));
        condition_idxs.push_back(temp_con);
      }
    }// 按照print_tuples的顺序构建的condition_idxs
    //TODO 元组的拼接需要实现笛卡尔积
    //TODO 将符合连接条件的元组添加到print_tables中

    // TupleSet test = std::move(tuple_sets[0]);
    // dikaerji(tuple_sets,res_tuples_set,0,tmp_tuple);
    std::vector<std::vector<Tuple>::const_iterator> dikaer_set;
    for(std::vector<TupleSet>::const_reverse_iterator
                it=tuple_sets.rbegin(),
                iend=tuple_sets.rend();
        it!=iend;it++){
      dikaer_set.push_back(it->tuples().begin());
      dikaer_set.push_back(it->tuples().end());
    }
    // std::cout<<dikaer_set.size()<<"\n";
    // dikaer_set:
    // [table1.begin,table1.end,table2.begin,table2.end,......,table3.begin,table3.end]
    // int table_count = dikaer_set.size()/2;
    std::vector<std::vector<Tuple>::const_iterator> temp_tuples;
    temp_tuples.clear();
    if(dikaer_set.size()==4){
      std::vector<Tuple>::const_iterator t1_begin = dikaer_set[0];
      std::vector<Tuple>::const_iterator t1_end = dikaer_set[1];
      std::vector<Tuple>::const_iterator t2_begin = dikaer_set[2];
      std::vector<Tuple>::const_iterator t2_end = dikaer_set[3];
      // std::vector<std::shared_ptr<TupleValue>> temp_values;
      for(t1_begin=dikaer_set[0];t1_begin!=t1_end;t1_begin++){
        for(t2_begin=dikaer_set[2];t2_begin!=t2_end;t2_begin++){
          temp_tuples.clear();
          temp_tuples.push_back(t1_begin);
          temp_tuples.push_back(t2_begin);
          Tuple res_tuple = merge_tuples(temp_tuples,select_order);
          if(match_join_condition(&res_tuple,condition_idxs)){
            print_tuples.add(std::move(res_tuple));
          }
        }
      }
    }
    if(dikaer_set.size()==6){
      std::vector<Tuple>::const_iterator t1_begin = dikaer_set[0];
      std::vector<Tuple>::const_iterator t1_end = dikaer_set[1];
      std::vector<Tuple>::const_iterator t2_begin = dikaer_set[2];
      std::vector<Tuple>::const_iterator t2_end = dikaer_set[3];
      std::vector<Tuple>::const_iterator t3_begin = dikaer_set[4];
      std::vector<Tuple>::const_iterator t3_end = dikaer_set[5];
      // std::vector<std::shared_ptr<TupleValue>> temp_values;
      for(t1_begin=dikaer_set[0];t1_begin!=t1_end;t1_begin++){
        for(t2_begin=dikaer_set[2];t2_begin!=t2_end;t2_begin++){
          for(t3_begin=dikaer_set[4];t3_begin!=t3_end;t3_begin++){
            temp_tuples.clear();
            temp_tuples.push_back(t1_begin);
            temp_tuples.push_back(t2_begin);
            temp_tuples.push_back(t3_begin);
            Tuple res_tuple = merge_tuples(temp_tuples,select_order);
            if(match_join_condition(&res_tuple,condition_idxs)){
              print_tuples.add(std::move(res_tuple));
            }
          }
        }
      }
    }
    // Tuple res_tuple = merge_tuples(dikaer_set,select_order);
    order_by_exec(selects,&print_tuples);
    print_tuples.print(ss);
  } else {
    // 当前只查询一张表，直接返回结果即可
    order_by_exec(selects,&tuple_sets.front());
    tuple_sets.front().print(ss);
  }

  for (SelectExeNode *&tmp_node : select_nodes) {
    delete tmp_node;
  }
  session_event->set_response(ss.str());
  end_trx_if_need(session, trx, true);
  return rc;
}

bool match_table(const Selects &selects, const char *table_name_in_condition, const char *table_name_to_match)
{
  if (table_name_in_condition != nullptr) {
    return 0 == strcmp(table_name_in_condition, table_name_to_match);
  }

  return selects.relation_num == 1;
}

static RC schema_add_field(Table *table, const char *field_name, TupleSchema &schema)
{
  const FieldMeta *field_meta = table->table_meta().field(field_name);
  if (nullptr == field_meta) {
    LOG_WARN("No such field. %s.%s", table->name(), field_name);
    return RC::SCHEMA_FIELD_MISSING;
  }

  schema.add_if_not_exists(field_meta->type(), table->name(), field_meta->name());
  return RC::SUCCESS;
}

// 把所有的表和只跟这张表关联的condition都拿出来，生成最底层的select 执行节点
RC create_selection_executor(
        Trx *trx, const Selects &selects, const char *db, const char *table_name, SelectExeNode &select_node)
{
  // 列出跟这张表关联的Attr
  TupleSchema schema;
  Table *table = DefaultHandler::get_default().find_table(db, table_name);
  if (nullptr == table) {
    LOG_WARN("No such table [%s] in db [%s]", table_name, db);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  for (int i = selects.attr_num - 1; i >= 0; i--) {
    const RelAttr &attr = selects.attributes[i];
    if (nullptr == attr.relation_name || 0 == strcmp(table_name, attr.relation_name)) {
      if (0 == strcmp("*", attr.attribute_name)) {
        // 列出这张表所有字段
        TupleSchema::from_table(table, schema);
        break;  // 没有校验，给出* 之后，再写字段的错误
      } else {
        // 列出这张表相关字段
        RC rc = schema_add_field(table, attr.attribute_name, schema);
        if (rc != RC::SUCCESS) {
          return rc;
        }
      }
    }
  }

  // 找出仅与此表相关的过滤条件, 或者都是值的过滤条件
  std::vector<DefaultConditionFilter *> condition_filters;
  for (size_t i = 0; i < selects.condition_num; i++) {
    const Condition &condition = selects.conditions[i];
    if ((condition.left_is_attr == 0 && condition.right_is_attr == 0) ||  // 两边都是值
        (condition.left_is_attr == 1 && condition.right_is_attr == 0 &&
         match_table(selects, condition.left_attr.relation_name, table_name)) ||  // 左边是属性右边是值
        (condition.left_is_attr == 0 && condition.right_is_attr == 1 &&
         match_table(selects, condition.right_attr.relation_name, table_name)) ||  // 左边是值，右边是属性名
        (condition.left_is_attr == 1 && condition.right_is_attr == 1 &&
         match_table(selects, condition.left_attr.relation_name, table_name) &&
         match_table(selects, condition.right_attr.relation_name, table_name))  // 左右都是属性名，并且表名都符合
            ) {
      DefaultConditionFilter *condition_filter = new DefaultConditionFilter();
      RC rc = condition_filter->init(*table, condition);
      if (rc != RC::SUCCESS) {
        delete condition_filter;
        for (DefaultConditionFilter *&filter : condition_filters) {
          delete filter;
        }
        return rc;
      }
      condition_filters.push_back(condition_filter);
    }
  }

  return select_node.init(trx, table, std::move(schema), std::move(condition_filters));
}