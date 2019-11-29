#ifndef general_performance_stats_viewer_GraphController_h
#define general_performance_stats_viewer_GraphController_h

#include "tp_maps/Controller.h"

namespace general_performance_stats_viewer
{

//##################################################################################################
class TP_MAPS_SHARED_EXPORT GraphController : public tp_maps::Controller
{
public:
  //################################################################################################
  GraphController(tp_maps::Map* map);

  //################################################################################################
  glm::vec3 focalPoint()const;

  //################################################################################################
  void setFocalPoint(const glm::vec3& focalPoint);

  //################################################################################################
  bool allowTranslation()const;

  //################################################################################################
  void setAllowTranslation(bool allowTranslation);

  //################################################################################################
  bool allowZoom()const;

  //################################################################################################
  void setAllowZoom(bool allowZoom);

  //##################################################################################################
  float rotationFactor()const;

  //##################################################################################################
  void setRotationFactor(float rotationFactor);

  //################################################################################################
  nlohmann::json saveState()const override;

  //################################################################################################
  void loadState(const nlohmann::json& j)override;

protected:
  //################################################################################################
  ~GraphController()override;

  //################################################################################################
   void mapResized(int w, int h)override;

  //################################################################################################
  void updateMatrices()override;

  //################################################################################################
  bool mouseEvent(const tp_maps::MouseEvent& event)override;

  //################################################################################################
  virtual void translate(float dx, float dy, double msSincePrevious);

  //################################################################################################
  virtual void translateInteractionFinished();

  //################################################################################################
  virtual void translateInteractionStarted();

private:
  struct Private;
  Private* d;
  friend struct Private;
};

}

#endif
