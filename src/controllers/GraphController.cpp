#include "general_performance_stats_viewer/controllers/GraphController.h"

#include "tp_maps/MouseEvent.h"
#include "tp_maps/Map.h"

#include "tp_math_utils/JSONUtils.h"
#include "tp_math_utils/Plane.h"

#include "tp_utils/JSONUtils.h"

#include "glm/gtx/transform.hpp"

namespace general_performance_stats_viewer
{
//##################################################################################################
struct GraphController::Private
{
  TP_REF_COUNT_OBJECTS("general_performance_stats_viewer::GraphController::Private");
  TP_NONCOPYABLE(Private);

  GraphController* q;

  float distanceX{10.0f};
  float distanceY{1.0f};

  glm::vec3 focalPoint{0, 0.5f, 0};

  //Behaviour
  bool allowTranslation{true};
  bool allowZoom{true};

  float rotationFactor{0.2f};

  glm::ivec2 previousPos{0,0};
  glm::ivec2 previousPos2{0,0};
  tp_maps::Button mouseInteraction{tp_maps::Button::NoButton};
  bool mouseMoved{false};

  //################################################################################################
  Private(GraphController* q_):
    q(q_)
  {

  }

  //################################################################################################
  void translate(float dx, float dy)
  {
    float radians = glm::radians(0.0f);
    float ca = std::cos(radians);
    float sa = std::sin(radians);

    //The width and height of the map widget
    float width  = float(q->map()->width());
    float height = float(q->map()->height());

    dx = dx / width;
    dy = dy / height;

    float fh = 1.0f;
    float fw = 1.0f;
    if(width>height)
    {
      fw = width/height;
    }
    else
    {
      fh = height/width;
    }

    dx *= (fw*distanceX)*2.0f;
    dy *= fh*distanceY*2.0f;

    focalPoint.x -= dx*ca - dy*sa;
    focalPoint.y += dx*sa + dy*ca;
  }
};

//##################################################################################################
GraphController::GraphController(tp_maps::Map* map):
  Controller(map),
  d(new Private(this))
{

}

//##################################################################################################
glm::vec3 GraphController::focalPoint()const
{
  return d->focalPoint;
}

//##################################################################################################
void GraphController::setFocalPoint(const glm::vec3& focalPoint)
{
  d->focalPoint = focalPoint;
  map()->update();
}

//##################################################################################################
bool GraphController::allowTranslation()const
{
  return d->allowTranslation;
}

//##################################################################################################
void GraphController::setAllowTranslation(bool allowTranslation)
{
  d->allowTranslation = allowTranslation;
}

//##################################################################################################
bool GraphController::allowZoom()const
{
  return d->allowZoom;
}

//##################################################################################################
void GraphController::setAllowZoom(bool allowZoom)
{
  d->allowZoom = allowZoom;
}

//##################################################################################################
float GraphController::rotationFactor()const
{
  return d->rotationFactor;
}

//##################################################################################################
void GraphController::setRotationFactor(float rotationFactor)
{
  d->rotationFactor = rotationFactor;
}

//##################################################################################################
nlohmann::json GraphController::saveState()const
{
  nlohmann::json j;

  j["Focal point"]    = tp_math_utils::vec3ToJSON(d->focalPoint);
  j["DistanceX"]      = d->distanceX;
  j["DistanceY"]      = d->distanceY;

  return j;
}

//##################################################################################################
void GraphController::loadState(const nlohmann::json& j)
{
  d->focalPoint    = tp_math_utils::getJSONVec3   (j, "Focal point"   , d->focalPoint   );
  d->distanceX     = tp_utils::getJSONValue<float>(j, "DistanceX"     , d->distanceX    );
  d->distanceY     = tp_utils::getJSONValue<float>(j, "DistanceY"     , d->distanceY    );

  map()->update();
}

//##################################################################################################
GraphController::~GraphController()
{
  delete d;
}

//##################################################################################################
void GraphController::mapResized(int w, int h)
{
  TP_UNUSED(w);
  TP_UNUSED(h);
}

//##################################################################################################
void GraphController::updateMatrices()
{
  if(std::fabs(d->distanceX) < 0.000000001f)
    d->distanceX = 1.0f;

  if(std::fabs(d->distanceY) < 0.000000001f)
    d->distanceY = 1.0f;

  //The width and height of the map widget
  float width  = float(map()->width());
  float height = float(map()->height());

  float fh = 1.0f;
  float fw = 1.0f;
  if(width>height)
    fw = width/height;
  else
    fh = height/width;

  glm::mat4 view = glm::mat4(1.0f);
  view = glm::translate(view, -d->focalPoint);

  glm::mat4 projection = glm::ortho(-fw*d->distanceX, // <- Left
                                    fw*d->distanceX,  // <- Right
                                    -fh*d->distanceY, // <- Bottom
                                    fh*d->distanceY,  // <- Top
                                    -1.0f,            // <- Near
                                    1.0f);            // <- Far
  Controller::Matrices vp;
  vp.p  = projection;
  vp.v  = view;
  vp.vp = projection * view;
  {
    glm::vec4 origin = glm::inverse(vp.vp) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vp.cameraOriginNear = origin / origin.w;
  }
  {
    glm::vec4 origin = glm::inverse(vp.vp) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    vp.cameraOriginFar = origin / origin.w;
  }
  setMatrices(tp_maps::defaultSID(), vp);
}

//##################################################################################################
bool GraphController::mouseEvent(const tp_maps::MouseEvent& event)
{
  const int mouseSensitivity=8;
  switch(event.type)
  {
  case tp_maps::MouseEventType::Press: //-----------------------------------------------------------
  {
    translateInteractionStarted();

    if(d->mouseInteraction == tp_maps::Button::NoButton)
    {
      d->mouseInteraction = event.button;
      d->previousPos = event.pos;
      d->mouseMoved = false;
    }
    break;
  }

  case tp_maps::MouseEventType::Move: //------------------------------------------------------------
  {
    glm::ivec2 pos = event.pos;

    if(!d->mouseMoved)
    {
      int ox = abs(d->previousPos.x - pos.x);
      int oy = abs(d->previousPos.y - pos.y);
      if((ox+oy) <= mouseSensitivity)
        break;
      d->mouseMoved = true;
    }

    float dx = float(pos.x - d->previousPos.x);
    float dy = float(pos.y - d->previousPos.y);

    d->previousPos2 = d->previousPos;
    d->previousPos = pos;

    if(d->mouseInteraction == tp_maps::Button::RightButton)
    {
      map()->update();
    }
    else if(d->mouseInteraction == tp_maps::Button::LeftButton && d->allowTranslation)
    {
      translate(dx, dy, 1);
      map()->update();
    }

    break;
  }

  case tp_maps::MouseEventType::Release: //---------------------------------------------------------
  {
    if(event.button == d->mouseInteraction)
    {
      d->mouseInteraction = tp_maps::Button::NoButton;

      if(!d->mouseMoved)
      {
        int ox = abs(d->previousPos.x - event.pos.x);
        int oy = abs(d->previousPos.y - event.pos.y);
        if((ox+oy) <= mouseSensitivity)
        {
          tp_maps::MouseEvent e = event;
          e.type = tp_maps::MouseEventType::Click;
          callMouseClickCallback(e);
        }
      }
      else if(event.button == tp_maps::Button::LeftButton)
      {
        translateInteractionFinished();
      }
    }
    break;
  }

  case tp_maps::MouseEventType::Wheel: //-----------------------------------------------------------
  {
    if(!d->allowZoom)
      return true;

    glm::vec3 scenePointA;
    bool moveOrigin = map()->unProject(event.pos, scenePointA, tp_math_utils::Plane());

    if(d->mouseInteraction == tp_maps::Button::RightButton)
    {
      if(event.delta<0)
        d->distanceY *= 1.1f;
      else if(event.delta>0)
        d->distanceY *= 0.9f;
      else
        return true;
    }
    else
    {
      if(event.delta<0)
        d->distanceX *= 1.1f;
      else if(event.delta>0)
        d->distanceX *= 0.9f;
      else
        return true;
    }

    if(moveOrigin)
    {
      updateMatrices();
      glm::vec3 scenePointB;
      moveOrigin = map()->unProject(event.pos, scenePointB, tp_math_utils::Plane());

      if(moveOrigin)
        d->focalPoint += scenePointA - scenePointB;
    }

    map()->update();
    break;
  }

  case tp_maps::MouseEventType::DoubleClick: //-----------------------------------------------------
  {
    d->mouseInteraction = tp_maps::Button::NoButton;
    break;
  }

  default: //---------------------------------------------------------------------------------------
  {
    d->mouseInteraction = tp_maps::Button::NoButton;
    break;
  }
  }

  return true;
}

//##################################################################################################
void GraphController::translate(float dx, float dy, double msSincePrevious)
{
  TP_UNUSED(msSincePrevious);
  d->translate(dx, dy);
}

//##################################################################################################
void GraphController::translateInteractionStarted()
{
}

//##################################################################################################
void GraphController::translateInteractionFinished()
{
}

}
