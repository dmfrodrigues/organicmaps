#include "qt/hline.hpp"

QHLine::QHLine(QWidget * parent) : QFrame(parent)
{
  setFrameShape(QFrame::HLine);
  setFrameShadow(QFrame::Sunken);
}
