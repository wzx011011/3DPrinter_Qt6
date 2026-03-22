#pragma once

#include <QString>

class QOpenGLShaderProgram;

namespace GLShaderUtil {

// Compile + link a vertex/fragment shader program with error logging.
// Returns true on success.  On failure, logs a warning and returns false.
bool compileShaderProgram(QOpenGLShaderProgram &prog,
                          const char *label,
                          const char *vertSrc,
                          const char *fragSrc);

} // namespace GLShaderUtil
