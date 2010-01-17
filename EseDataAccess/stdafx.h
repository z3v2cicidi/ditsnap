// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Windows ヘッダーから使用されていない部分を除外します。
#include <stdio.h>
#include <tchar.h>


// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
#include "stdlib.h"
#include <vector>
#include <string>

#include "windows.h"
#include "esent.h"

using std::vector;
using std::string;
using std::wstring;

typedef unsigned int uint;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
TypeName(const TypeName&);                 \
void operator=(const TypeName&)
