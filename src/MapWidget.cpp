#include "general_performance_stats_viewer/MapWidget.h"

#include "tp_maps/PickingResult.h"
#include "tp_maps/RenderInfo.h"
#include "tp_maps/picking_results/PointsPickingResult.h"
#include "tp_maps/picking_results/LinesPickingResult.h"

#include <QHelpEvent>

#include <memory>
#include <iostream>

namespace general_performance_stats_viewer
{

//##################################################################################################
struct MapWidget::Private
{
  TP_REF_COUNT_OBJECTS("general_performance_stats_viewer::MapWidget::Private");
  TP_NONCOPYABLE(Private);

  MapWidget* q;

  //################################################################################################
  Private(MapWidget* q_):
    q(q_)
  {

  }
};

//##################################################################################################
MapWidget::MapWidget():
  d(new Private(this))
{

}

//##################################################################################################
MapWidget::~MapWidget()
{
  delete d;
}

//##################################################################################################
bool MapWidget::event(QEvent* event)
{
  if (event->type() == QEvent::ToolTip)
  {
      QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

      std::unique_ptr<tp_maps::PickingResult> pickingResult{map()->performPicking(toolTipSID(), {helpEvent->x(), helpEvent->y()})};
      if(!pickingResult)
      {
        event->ignore();
        return true;
      }

      {
        auto pointsLayerPickingResult = dynamic_cast<tp_maps::PointsPickingResult*>(pickingResult.get());
        if(pointsLayerPickingResult)
        {
          emit pointsLayerToolTipEvent(helpEvent, pointsLayerPickingResult);
          return true;
        }
      }


      {
        auto linesLayerPickingResult = dynamic_cast<tp_maps::LinesPickingResult*>(pickingResult.get());
        if(linesLayerPickingResult)
        {
          emit linesLayerToolTipEvent(helpEvent, linesLayerPickingResult);
          return true;
        }
      }

      event->ignore();
      return true;
  }
  return QWidget::event(event);
}

}
