#include "general_performance_stats_viewer/MainWindow.h"

#include "tp_qt_maps_widget/MapWidget.h"

#include "tp_maps/layers/PointsLayer.h"
#include "tp_maps/textures/DefaultSpritesTexture.h"
#include "tp_maps/layers/LinesLayer.h"

#include <QBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMenu>
#include <QCursor>

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

  tp_qt_maps_widget::MapWidget* mapWidget{nullptr};

  std::vector<tp_maps::Layer*> pointLayers;
  std::vector<tp_maps::Layer*> lineLayers;

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

      trace->points.push_back({i, value});

      if(value>trace->maxValue)
        trace->maxValue = value;
    }

    tpDeleteAll(pointLayers);
    pointLayers.clear();

    tpDeleteAll(lineLayers);
    lineLayers.clear();

    listWidget->clear();

    size_t t=0;
    for(const auto& [name, trace] : traces)
    {
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
        auto spriteTexture = new tp_maps::SpriteTexture();
        spriteTexture->setTexture(new tp_maps::DefaultSpritesTexture(mapWidget->map()));
        auto layer = new tp_maps::PointsLayer(spriteTexture);
        layer->setPoints(points);
        mapWidget->map()->addLayer(layer);
        pointLayers.push_back(layer);
      }

      {
        auto layer = new tp_maps::LinesLayer();
        layer->setLines({line});
        mapWidget->map()->addLayer(layer);
        lineLayers.push_back(layer);
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
};

//##################################################################################################
MainWindow::MainWindow():
  d(new Private(this))
{
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
  connect(d->listWidgetMenu->addAction("Show selected"), &QAction::triggered, [&]{d->showSelected();});
  connect(d->listWidgetMenu->addAction("Hide selected"), &QAction::triggered, [&]{d->hideSelected();});
  connect(d->listWidgetMenu->addAction("Show all"),      &QAction::triggered, [&]{d->showAll();     });
  connect(d->listWidgetMenu->addAction("Hide all"),      &QAction::triggered, [&]{d->hideAll();     });

  auto loadButton = new QPushButton("Load");
  leftLayout->addWidget(loadButton);
  connect(loadButton, &QAbstractButton::clicked, [&]{d->load();});

  d->mapWidget = new tp_qt_maps_widget::MapWidget();
  splitter->addWidget(d->mapWidget);

  splitter->setSizes({1000, 6000});
}

//##################################################################################################
MainWindow::~MainWindow()
{
  delete d;
}

}
