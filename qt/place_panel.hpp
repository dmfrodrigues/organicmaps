#pragma once

#include "search/reverse_geocoder.hpp"

#include <qwidget.h>
#include "map/framework.hpp"

namespace place_page
{
class Info;
}

class PlacePanel : public QWidget
{
  Q_OBJECT
public:
  PlacePanel(QWidget * parent);

  void setPlace(
    place_page::Info const & info,
    search::ReverseGeocoder::Address const & address
  );

signals:
  void showPlace();
  void hidePlace();
  void editPlace(place_page::Info const & info);

private:
  place_page::Info const * infoPtr;

  void OnEdit();
};
