#ifndef general_performance_stats_viewer_MainWindow_h
#define general_performance_stats_viewer_MainWindow_h

#include "general_performance_stats_viewer/Globals.h"

#include <QWidget>

namespace general_performance_stats_viewer
{

//##################################################################################################
class MainWindow : public QWidget
{
public:
  //################################################################################################
  MainWindow();

  //################################################################################################
  ~MainWindow() override;

private:
  struct Private;
  friend struct Private;
  Private* d;
};

}

#endif
