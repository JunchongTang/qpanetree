#pragma once

#include <QLoggingCategory>

namespace QPaneTree {

// 日志范畴 "panetree"。默认级别 QtWarningMsg，即默认只输出 warning 及以上；
// debug / info 默认关闭。用户在外面打开调试输出：
//   QLoggingCategory::setFilterRules("panetree.debug=true");
// 或环境变量：
//   QT_LOGGING_RULES="panetree.debug=true"
Q_DECLARE_LOGGING_CATEGORY(lcPaneTree)

} // namespace QPaneTree
