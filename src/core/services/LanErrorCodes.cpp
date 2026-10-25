#include "LanErrorCodes.h"

#include <QHash>

namespace {
void seed(QHash<int, QString> &table)
{
    // 仅收录可从开源材料确证的编码（bundle deviceMgr：repoPlrStatus↔errcode
    // 115 的双向映射是 JS 里唯一可见的数字语义）。其余编码表在机型固件/
    // 服务侧，开源仓库不含——未知码显示数字本身，绝不编造原因（审计纪律）。
    table.insert(115, QStringLiteral("耗材断料/请检查进料"));
}
} // namespace

QString LanErrorCodes::text(int errcode)
{
    if (errcode == 0)
        return {};
    static const QHash<int, QString> kTable = [] { QHash<int, QString> t; seed(t); return t; }();
    const auto it = kTable.constFind(errcode);
    if (it != kTable.constEnd())
        return *it;
    return QStringLiteral("设备报告错误（错误码 %1）").arg(errcode);
}

bool LanErrorCodes::isKnown(int errcode)
{
    static const QHash<int, QString> kTable = [] { QHash<int, QString> t; seed(t); return t; }();
    return errcode != 0 && kTable.contains(errcode);
}
