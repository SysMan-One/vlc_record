/*------------------------------ Infor mation ---------------------------*//**
 *
 *  $HeadURL$
 *
 *  @file     qstringfilterwidgetaction.h
 *
 *  @author   Jo2003
 *
 *  @date     29.07.2013
 *
 *  $Id$
 *
 *///------------------------- (c) 2013 by Jo2003  --------------------------
#ifndef __20130729_QSTRINGFILTERWIDGETACTION_H
   #define __20130729_QSTRINGFILTERWIDGETACTION_H

#include <QWidgetAction>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QEvent>
#include <QPushButton>
#include <QTimer>

//---------------------------------------------------------------------------
//! \class   QStringFilterWidgetAction
//! \date    29.07.2013
//! \author  Jo2003
//! \brief   widget for channel filter
//---------------------------------------------------------------------------
class QStringFilterWidgetAction : public QWidgetAction
{
   Q_OBJECT

public:
   //---------------------------------------------------------------------------
   //
   //! \brief   constructs QStringFilterWidgetAction object
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   parent (QObject *) pointer to parent object
   //
   //! \return  --
   //---------------------------------------------------------------------------
   QStringFilterWidgetAction(QObject *parent = 0)
      : QWidgetAction(parent), _lab(NULL), _line(NULL)
   {
      _tCheck.setSingleShot(true);
      _tCheck.setInterval(500);

      connect(&_tCheck, SIGNAL(timeout()), this, SLOT(slotFilter()));
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   clear filter string
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   --
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void cleanFilter()
   {
      _line->clear();
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   set focus to line and place cursor at the end of text
   //
   //! \author  Jo2003
   //! \date    31.07.2013
   //
   //! \param   --
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void lineFocus()
   {
      _line->setCursorPosition(_line->text().length());
      _line->setFocus(Qt::OtherFocusReason);
   }

private:
   QLabel      *_lab;
   QLineEdit   *_line;
   QTimer       _tCheck;

protected:
   //---------------------------------------------------------------------------
   //
   //! \brief   create channel filter widget
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   parent (QWidget *) pointer to parent widget
   //
   //! \return  --
   //---------------------------------------------------------------------------
   virtual QWidget* createWidget (QWidget * parent)
   {
      QWidget     *w = new QWidget(parent);
      QHBoxLayout *l = new QHBoxLayout();

      _lab           = new QLabel(tr("Channel filter: "), w);
      _line          = new QLineEdit(w);

      l->setSpacing(2);
      l->setMargin(4);

      l->addWidget(_lab);
      l->addWidget(_line, 10);

      w->setLayout(l);

      connect(_line, SIGNAL(textEdited(QString)), &_tCheck, SLOT(start()));
      connect(_line, SIGNAL(returnPressed()), parent, SLOT(hide()));

      return w;
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   handle language switch event
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   e (QEvent *) pointer to event
   //
   //! \return  true -> handled; false -> not handled
   //---------------------------------------------------------------------------
   virtual bool event(QEvent *e)
   {
      bool rv = false;

      if (e->type() == QEvent::LanguageChange)
      {
         // make sure we already created
         // control elements ...
         if (_lab)
         {
            _lab->setText(tr("Channel filter: "));

            rv = true;
         }
      }
      else
      {
         rv = QWidgetAction::event(e);
      }

      return rv;
   }

private slots:
   //---------------------------------------------------------------------------
   //
   //! \brief   ok button or enter was pressed, trigger sigFilter
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   --
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void slotFilter()
   {
      // slot is reached only if we've created
      // control elements already ...
      emit sigFilter(_line->text());
   }

signals:
   //---------------------------------------------------------------------------
   //
   //! \brief   signal filter string
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   QString filter string
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void sigFilter(QString);
};

#endif // __20130729_QSTRINGFILTERWIDGETACTION_H
