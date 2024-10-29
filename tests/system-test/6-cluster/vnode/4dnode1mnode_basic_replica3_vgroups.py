# author : wenzhouwww
from ssl import ALERT_DESCRIPTION_CERTIFICATE_UNOBTAINABLE
import taos
import sys
import time
import os

from util.log import *
from util.sql import *
from util.cases import *
from util.dnodes import TDDnodes
from util.dnodes import TDDnode
from util.cluster import *

import time
import socket
import subprocess

class TDTestCase:
    def init(self, conn, logSql, replicaVar=1):
        self.replicaVar = int(replicaVar)
        tdLog.debug(f"start to excute {__file__}")
        tdSql.init(conn.cursor())
        self.host = socket.gethostname()
        self.mnode_list = {}
        self.dnode_list = {}
        self.ts = 1483200000000
        self.db_name ='testdb'
        self.replica = 1
        self.vgroups = 2
        self.tb_nums = 10
        self.row_nums = 10
        self.max_vote_time_cost = 100  # seconds

    def getBuildPath(self):
        selfPath = os.path.dirname(os.path.realpath(__file__))
        if ("community" in selfPath):
            projPath = selfPath[:selfPath.find("community")]
        else:
            projPath = selfPath[:selfPath.find("tests")]

        for root, dirs, files in os.walk(projPath):
            if ("taosd" in files or "taosd.exe" in files):
                rootRealPath = os.path.dirname(os.path.realpath(root))
                if ("packaging" not in rootRealPath):
                    buildPath = root[:len(root) - len("/build/bin")]
                    break
        return buildPath

    def check_setup_cluster_status(self):
        tdSql.query("select * from information_schema.ins_mnodes")
        for mnode in tdSql.queryResult:
            name = mnode[1]
            info = mnode
            self.mnode_list[name] = info

        tdSql.query("select * from information_schema.ins_dnodes")
        for dnode in tdSql.queryResult:
            name = dnode[1]
            info = dnode
            self.dnode_list[name] = info

        count = 0
        is_leader = False
        mnode_name = ''
        for k,v in self.mnode_list.items():
            count +=1
            # only for 1 mnode
            mnode_name = k

            if v[2] =='leader':
                is_leader=True

        if count==1 and is_leader:
            tdLog.notice("===== depoly cluster success with 1 mnode as leader =====")
        else:
            tdLog.notice("===== depoly cluster fail with 1 mnode as leader =====")

        for k ,v in self.dnode_list.items():
            if k == mnode_name:
                if v[3]==0:
                    tdLog.notice("===== depoly cluster mnode only success at {} , support_vnodes is {} ".format(mnode_name,v[3]))
                else:
                    tdLog.notice("===== depoly cluster mnode only fail at {} , support_vnodes is {} ".format(mnode_name,v[3]))
            else:
                continue

    def create_db_check_vgroups(self):

        tdSql.execute("drop database if exists test")
        tdSql.execute("create database if not exists test replica 1 duration 100")
        tdSql.execute("use test")
        tdSql.execute(
        '''create table stb1
        (ts timestamp, c1 int, c2 bigint, c3 smallint, c4 tinyint, c5 float, c6 double, c7 bool, c8 binary(16),c9 nchar(32), c10 timestamp)
        tags (t1 int)
        '''
        )
        tdSql.execute(
            '''
            create table t1
            (ts timestamp, c1 int, c2 bigint, c3 smallint, c4 tinyint, c5 float, c6 double, c7 bool, c8 binary(16),c9 nchar(32), c10 timestamp)
            '''
        )

        for i in range(5):
            tdSql.execute("create table sub_tb_{} using stb1 tags({})".format(i,i))
        tdSql.query("show stables")
        tdSql.checkRows(1)
        tdSql.query("show tables")
        tdSql.checkRows(6)

        tdSql.query("show test.vgroups;")
        vgroups_infos = {}  # key is id: value is info list
        for vgroup_info in tdSql.queryResult:
            vgroup_id = vgroup_info[0]
            tmp_list = []
            for role in vgroup_info[3:-4]:
                if role in ['leader', 'leader*', 'leader**', 'follower']:
                    tmp_list.append(role)
            vgroups_infos[vgroup_id]=tmp_list

        for k , v in vgroups_infos.items():
            if len(v) == 1 and v[0] in ['leader', 'leader*', 'leader**']:
                tdLog.notice(" === create database replica only 1 role leader  check success of vgroup_id {} ======".format(k))
            else:
                tdLog.notice(" === create database replica only 1 role leader  check fail of vgroup_id {} ======".format(k))

    def check_vgroups_init_done(self,dbname):

        status = True

        tdSql.query("show {}.vgroups".format(dbname))
        for vgroup_info in tdSql.queryResult:
            vgroup_id = vgroup_info[0]
            vgroup_status = []
            for ind , role in enumerate(vgroup_info[3:-4]):

                if ind%2==0:
                    continue
                else:
                    vgroup_status.append(role)
            if vgroup_status.count("leader")!=1 or vgroup_status.count("follower")!=2:
                status = False
                return status
        return status


    def vote_leader_time_costs(self,dbname):
        start = time.time()
        status = self.check_vgroups_init_done(dbname)
        while not status:
            time.sleep(0.1)
            status = self.check_vgroups_init_done(dbname)

            # tdLog.notice("=== database {} show vgroups vote the leader is in progress ===".format(dbname))
        end = time.time()
        cost_time = end - start
        tdLog.notice(" ==== database %s vote the leaders success , cost time is %.3f second ====="%(dbname,cost_time) )
        # os.system("taos -s 'show {}.vgroups;'".format(dbname))
        if cost_time >= self.max_vote_time_cost:
            tdLog.notice(" ==== database %s vote the leaders cost too large time , cost time is %.3f second ===="%(dbname,cost_time) )


        return cost_time

    def test_init_vgroups_time_costs(self):

        tdLog.notice(" ====start check time cost about vgroups vote leaders ==== ")
        tdLog.notice(" ==== current max time cost is set value : {} =======".format(self.max_vote_time_cost))

        # create database replica 3 vgroups 1

        db1 = 'db_1'
        create_db_replica_3_vgroups_1 = "create database {} replica 3 vgroups 1".format(db1)
        tdLog.notice('=======database {} replica 3 vgroups 1 ======'.format(db1))
        tdSql.execute(create_db_replica_3_vgroups_1)
        self.vote_leader_time_costs(db1)

        # create database replica 3 vgroups 10
        db2 = 'db_2'
        create_db_replica_3_vgroups_10 = "create database {} replica 3 vgroups 10".format(db2)
        tdLog.notice('=======database {} replica 3 vgroups 10 ======'.format(db2))
        tdSql.execute(create_db_replica_3_vgroups_10)
        self.vote_leader_time_costs(db2)

        # create database replica 3 vgroups 30
        db3 = 'db_3'
        create_db_replica_3_vgroups_100 = "create database {} replica 3 vgroups 20".format(db3)
        tdLog.notice('=======database {} replica 3 vgroups 30 ======'.format(db3))
        tdSql.execute(create_db_replica_3_vgroups_100)
        self.vote_leader_time_costs(db3)



    def run(self):
        self.check_setup_cluster_status()
        self.test_init_vgroups_time_costs()



    def stop(self):
        tdSql.close()
        tdLog.success(f"{__file__} successfully executed")

tdCases.addLinux(__file__, TDTestCase())
tdCases.addWindows(__file__, TDTestCase())
