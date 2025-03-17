// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once


#define LOCAL_NAME "PANU"

#define BT_PAN_SERVICE_MSG_COUNT          (60)
#define BT_PAN_SERVICE_TASK_PRIORITY      (4)

#define PAGE_SCAN_INTV   0x0800  //1.28s
#define PAGE_SCAN_WIN    0x00B4  //11.25ms

#define CONFIG_PAGE_TIMEOUT  16000   //unit of 0.625ms
#define CONFIG_RECONN_INTERVAL  2000   //unit of 1ms

#define CONFIG_NB_ACL_BUFF  4
#define CONFIG_MAX_TX_CACHE_COUNT  200
#define CONFIG_MAX_RECONN_COUNT  3

