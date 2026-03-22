#include "GLShaderUtil.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QDebug>

namespace GLShaderUtil {

bool compileShaderProgram(QOpenGLShaderProgram &prog,
                          const char *label,
                          const char *vertSrc,
                          const char *fragSrc)
{
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc)) {
        qWarning("[GL] %s vertex compile failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
        qWarning("[GL] %s fragment compile failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    if (!prog.link()) {
        qWarning("[GL] %s link failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    return true;
}

} // namespace GLShaderUtil
