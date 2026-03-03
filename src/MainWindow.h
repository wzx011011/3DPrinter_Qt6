#pragma once

#include <QList>
#include <QPoint>
#include <QWidget>

class QFrame;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTimer;

class MainWindow final : public QWidget
{
public:
  explicit MainWindow(QWidget *parent = nullptr);

protected:
  void closeEvent(QCloseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  void buildUi();
  void switchPage(int index);
  void refreshNavState();
  void loadWindowState();
  void saveWindowState() const;

  QFrame *titleBar_ = nullptr;
  QStackedWidget *stack_ = nullptr;
  QList<QPushButton *> navButtons_;
  int currentIndex_ = 0;

  QLabel *statusLabel_ = nullptr;
  QLabel *pageLabel_ = nullptr;
  QLabel *clockLabel_ = nullptr;
  QLabel *materialLabel_ = nullptr;
  QTimer *clockTimer_ = nullptr;

  bool dragging_ = false;
  QPoint dragOffset_;
};
