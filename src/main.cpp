#include "general_performance_stats_viewer/MainWindow.h"

#include <QApplication>

using namespace general_performance_stats_viewer;

//##################################################################################################
int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  MainWindow mainWindow;
  mainWindow.showMaximized();
  return app.exec();
}
