#ifndef general_performance_stats_viewer_MapWidget_h
#define general_performance_stats_viewer_MapWidget_h

#include "general_performance_stats_viewer/Globals.h"

#include "tp_qt_maps_widget/MapWidget.h"

class QHelpEvent;

namespace tp_maps
{
class PointsPickingResult;
class LinesPickingResult;
}

namespace general_performance_stats_viewer
{

//##################################################################################################
class MapWidget : public tp_qt_maps_widget::MapWidget
{
  Q_OBJECT
public:
  //################################################################################################
  MapWidget();

  //################################################################################################
  ~MapWidget() override;

  //################################################################################################
  Q_SIGNAL void pointsLayerToolTipEvent(QHelpEvent* helpEvent, tp_maps::PointsPickingResult* result);

  //################################################################################################
  Q_SIGNAL void linesLayerToolTipEvent(QHelpEvent* helpEvent, tp_maps::LinesPickingResult* result);

protected:
  //################################################################################################
  bool event(QEvent* event) override;

private:
  struct Private;
  friend struct Private;
  Private* d;
};

}

#endif
