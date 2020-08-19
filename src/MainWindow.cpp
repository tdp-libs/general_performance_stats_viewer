#include "general_performance_stats_viewer/MainWindow.h"
#include "general_performance_stats_viewer/MapWidget.h"

#include "tp_maps/controllers/GraphController.h"
#include "tp_maps/layers/PointsLayer.h"
#include "tp_maps/layers/LinesLayer.h"
#include "tp_maps/textures/DefaultSpritesTexture.h"
#include "tp_maps/picking_results/PointsPickingResult.h"
#include "tp_maps/picking_results/LinesPickingResult.h"

#include "tp_utils/DebugUtils.h"

#include <QBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMenu>
#include <QCursor>
#include <QToolTip>
#include <QHelpEvent>

#include <fstream>
#include <iostream>
#include <memory>

namespace general_performance_stats_viewer
{

namespace
{
struct TraceDetails_lt
{
  size_t maxValue{1};
  std::vector<std::pair<size_t, size_t>> points;
};
}

//##################################################################################################
struct MainWindow::Private
{
  TP_REF_COUNT_OBJECTS("general_performance_stats_viewer::MainWindow::Private");
  TP_NONCOPYABLE(Private);

  MainWindow* q;

  QListWidget* listWidget{nullptr};
  QMenu* listWidgetMenu{nullptr};

  general_performance_stats_viewer::MapWidget* mapWidget{nullptr};
  tp_maps::GraphController* graphController{nullptr};

  std::vector<tp_maps::Layer*> pointLayers;
  std::vector<tp_maps::Layer*> lineLayers;
  std::vector<std::vector<size_t>> originalValues;

  //################################################################################################
  Private(MainWindow* q_):
    q(q_)
  {

  }

  //################################################################################################
  void load()
  {
    auto path = QFileDialog::getOpenFileName(q, "Select process stats file");
    if(path.isEmpty())
      return;

    size_t pointCount{0};
    std::map<std::string, std::shared_ptr<TraceDetails_lt>> traces;

    std::ifstream infile(path.toStdString());
    size_t i=0;
    for(std::string line; std::getline(infile, line);)
    {
      if(line == "==================")
      {
        i++;
        continue;
      }

      std::vector<std::string> parts;
      tpSplit(parts, line, " ---> ", tp_utils::SplitBehavior::SkipEmptyParts);

      if(parts.size()!=2)
        continue;

      std::string name = parts.at(0);
      auto value = size_t(std::stoull(parts.at(1)));

      if(name.empty())
        continue;

      auto& trace = traces[name];
      if(!trace)
        trace = std::make_shared<TraceDetails_lt>();

      pointCount++;
      trace->points.push_back({i, value});

      if(value>trace->maxValue)
        trace->maxValue = value;
    }

    tpWarning() << "Loaded " << pointCount << " data points.";

    tpDeleteAll(pointLayers);
    pointLayers.clear();

    tpDeleteAll(lineLayers);
    lineLayers.clear();    

    originalValues.clear();

    listWidget->clear();

    std::vector<std::string> names;
    for(const auto& t : traces)
      names.push_back(t.first);

    std::sort(names.begin(), names.end(),[](const auto& a, const auto& b){return QString::fromStdString(a).compare(QString::fromStdString(b), Qt::CaseInsensitive)<0;});

    size_t t=0;
    originalValues.resize(names.size());
    for(const auto& name : names)
    {
      const auto& trace = traces[name];

      int hue = int(float(t) / float(traces.size()) * 360.0f);
      QColor color = QColor::fromHsl(hue, 255, 128);
      glm::vec4 colorF(color.redF(), color.greenF(), color.blueF(), 1.0f);

      auto item = new QListWidgetItem(QString::fromStdString(name));
      item->setBackground(QBrush(color));
      item->setCheckState(Qt::Checked);
      listWidget->addItem(item);

      //Style the points and prepare them for rendering.
      std::vector<tp_maps::PointSpriteShader::PointSprite> points;
      tp_maps::Lines line;
      line.mode = GL_LINE_STRIP;
      line.color = colorF;
      points.resize(trace->points.size());
      line.lines.resize(trace->points.size());
      for(size_t p=0; p<trace->points.size(); p++)
      {
        const auto& src = trace->points.at(p);
        auto& dst = points.at(p);
        dst.position = glm::vec3(float(src.first) / float(i) * 8.0f, float(src.second) / float(trace->maxValue), 0.0f);
        dst.color = colorF;
        dst.radius = 2.5f;
        line.lines.at(p) = dst.position;
      }

      {
        auto layer = new tp_maps::LinesLayer();
        layer->setDefaultRenderPass(tp_maps::RenderPass::GUI);
        layer->setLines({line});
        mapWidget->map()->addLayer(layer);
        lineLayers.push_back(layer);
      }

      {
        auto spriteTexture = new tp_maps::SpriteTexture();
        spriteTexture->setTexture(new tp_maps::DefaultSpritesTexture(mapWidget->map()));
        auto layer = new tp_maps::PointsLayer(spriteTexture);
        layer->setDefaultRenderPass(tp_maps::RenderPass::GUI);
        layer->setPoints(points);
        mapWidget->map()->addLayer(layer);
        pointLayers.push_back(layer);
      }

      {
        auto& p = originalValues.at(t);
        p.reserve(trace->points.size());
        for(const auto& point : trace->points)
          p.push_back(point.second);
      }

      t++;
    }

    mapWidget->map()->update();
  }

  //################################################################################################
  void itemChanged(QListWidgetItem* item)
  {
    auto row = size_t(listWidget->row(item));
    if(row<pointLayers.size())
    {
      pointLayers.at(row)->setVisible(item->checkState() == Qt::Checked);
      mapWidget->map()->update();
    }

    if(row<lineLayers.size())
    {
      lineLayers.at(row)->setVisible(item->checkState() == Qt::Checked);
      mapWidget->map()->update();
    }
  }

  //################################################################################################
  void showSelected()
  {
    for(auto i : listWidget->selectedItems())
      i->setCheckState(Qt::Checked);
  }

  //################################################################################################
  void hideSelected()
  {
    for(auto i : listWidget->selectedItems())
      i->setCheckState(Qt::Unchecked);
  }

  //################################################################################################
  void showAll()
  {
    for(int i=0; i<listWidget->count(); i++)
      listWidget->item(i)->setCheckState(Qt::Checked);
  }

  //################################################################################################
  void hideAll()
  {
    for(int i=0; i<listWidget->count(); i++)
      listWidget->item(i)->setCheckState(Qt::Unchecked);
  }

  //################################################################################################
  void hideAllExceptSelected()
  {
    auto selected = listWidget->selectedItems();

    for(int i=0; i<listWidget->count(); i++)
    {
      auto item = listWidget->item(i);
      item->setCheckState(selected.contains(item)?Qt::Checked:Qt::Unchecked);
    }
  }

  //################################################################################################
  void bringItemToFront(QListWidgetItem* item)
  {
    auto row = size_t(listWidget->row(item));
    if(row<lineLayers.size())
    {
      auto layer = lineLayers.at(row);
      mapWidget->map()->removeLayer(layer);
      mapWidget->map()->addLayer(layer);
    }

    if(row<pointLayers.size())
    {
      auto layer = pointLayers.at(row);
      mapWidget->map()->removeLayer(layer);
      mapWidget->map()->addLayer(layer);
    }

    mapWidget->map()->update();
  }

  //################################################################################################
  void bringToFront()
  {
    for(auto i : listWidget->selectedItems())
      bringItemToFront(i);
  }

  //################################################################################################
  void pointsLayerToolTipEvent(QHelpEvent* helpEvent, tp_maps::PointsPickingResult* result)
  {
    TP_UNUSED(helpEvent);

    for(size_t i=0; i<pointLayers.size(); i++)
    {
      auto l = pointLayers.at(i);
      if(l == result->pointsLayer)
      {
        if(i>=size_t(listWidget->count()))
          break;

        auto item = listWidget->item(int(i));

        if(i>=originalValues.size())
          break;

        const auto& values = originalValues.at(i);

        if(result->index>=values.size())
          break;

        QToolTip::showText(helpEvent->globalPos(), QString("(%1) %2").arg(values.at(result->index)).arg(item->text()));

        break;
      }
    }
  }

  //################################################################################################
  void linesLayerToolTipEvent(QHelpEvent* helpEvent, tp_maps::LinesPickingResult* result)
  {
    TP_UNUSED(helpEvent);

    for(size_t i=0; i<lineLayers.size(); i++)
    {
      auto l = lineLayers.at(i);
      if(l == result->linesLayer)
      {
        if(i>=size_t(listWidget->count()))
          break;

        auto item = listWidget->item(int(i));

        QToolTip::showText(helpEvent->globalPos(), item->text());

        break;
      }
    }
  }
};

//##################################################################################################
MainWindow::MainWindow():
  d(new Private(this))
{
  setWindowTitle("Performance Stats Viewer");

  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0,0,0,0);

  auto splitter = new QSplitter();
  mainLayout->addWidget(splitter);

  auto leftWidget = new QWidget();
  splitter->addWidget(leftWidget);
  auto leftLayout = new QVBoxLayout(leftWidget);
  leftLayout->setContentsMargins(6,0,0,6);

  d->listWidget = new QListWidget();
  leftLayout->addWidget(d->listWidget);
  connect(d->listWidget, &QListWidget::itemChanged, [&](QListWidgetItem* item){d->itemChanged(item);});
  d->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  d->listWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  d->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(d->listWidget, &QWidget::customContextMenuRequested, [&](const QPoint& pos){d->listWidgetMenu->exec(d->listWidget->mapToGlobal(pos));});

  d->listWidgetMenu = new QMenu(d->listWidget);
  connect(d->listWidgetMenu->addAction("Show selected"),            &QAction::triggered, [&]{d->showSelected();         });
  connect(d->listWidgetMenu->addAction("Hide selected"),            &QAction::triggered, [&]{d->hideSelected();         });
  connect(d->listWidgetMenu->addAction("Show all"),                 &QAction::triggered, [&]{d->showAll();              });
  connect(d->listWidgetMenu->addAction("Hide all"),                 &QAction::triggered, [&]{d->hideAll();              });
  connect(d->listWidgetMenu->addAction("Hide all except selected"), &QAction::triggered, [&]{d->hideAllExceptSelected();});
  connect(d->listWidgetMenu->addAction("Bring to front"),           &QAction::triggered, [&]{d->bringToFront();         });

  auto loadButton = new QPushButton("Load");
  leftLayout->addWidget(loadButton);
  connect(loadButton, &QAbstractButton::clicked, [&]{d->load();});

  d->mapWidget = new general_performance_stats_viewer::MapWidget();
  splitter->addWidget(d->mapWidget);

  connect(d->mapWidget, &general_performance_stats_viewer::MapWidget::pointsLayerToolTipEvent, [&](QHelpEvent* helpEvent, tp_maps::PointsPickingResult* result){d->pointsLayerToolTipEvent(helpEvent, result);});
  connect(d->mapWidget, &general_performance_stats_viewer::MapWidget::linesLayerToolTipEvent, [&](QHelpEvent* helpEvent, tp_maps::LinesPickingResult* result){d->linesLayerToolTipEvent(helpEvent, result);});

  d->graphController = new tp_maps::GraphController(d->mapWidget->map());

  splitter->setSizes({1000, 6000});
}

//##################################################################################################
MainWindow::~MainWindow()
{
  delete d;
}

}
