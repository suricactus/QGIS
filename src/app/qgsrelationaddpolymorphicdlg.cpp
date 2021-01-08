/***************************************************************************
    qgsrelationaddpolymorphicdlg.cpp
    ---------------------
    begin                : December 2020
    copyright            : (C) 2020 by Ivan Ivanov
    email                : ivan at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDialogButtonBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "qgsrelationaddpolymorphicdlg.h"
#include "qgsvectorlayer.h"
#include "qgsmaplayercombobox.h"
#include "qgsfieldcombobox.h"
#include "qgsmaplayerproxymodel.h"
#include "qgsapplication.h"
#include "qgshelp.h"
#include "qgsproject.h"
#include "qgsrelationmanager.h"
#include "qgsfieldexpressionwidget.h"


QgsRelationAddPolymorphicDlg::QgsRelationAddPolymorphicDlg( QWidget *parent )
  : QDialog( parent )
  , Ui::QgsRelationManagerAddPolymorphicDialogBase()
{
  setupUi( this );
  setWindowTitle( tr( "Add New Polymorphic Relation" ) );

  mButtonBox->setStandardButtons( QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::Ok );
  connect( mButtonBox, &QDialogButtonBox::accepted, this, &QgsRelationAddPolymorphicDlg::accept );
  connect( mButtonBox, &QDialogButtonBox::rejected, this, &QgsRelationAddPolymorphicDlg::reject );
  connect( mButtonBox, &QDialogButtonBox::helpRequested, this, [ = ]
  {
    QgsHelp::openHelp( QStringLiteral( "working_with_vector/attribute_table.html#defining-polymorphic-relations" ) );
  } );

  const QVector<QgsVectorLayer *> layers = QgsProject::instance()->layers<QgsVectorLayer *>();
  for ( const QgsMapLayer *vl : layers )
  {
    if ( !vl || !vl->isValid() )
      continue;

    mReferencedLayersComboBox->addItem( vl->name(), vl->id() );
  }

  addFieldsRow();
  updateTypeConfigWidget();
  updateDialogButtons();
  updateReferencedLayerFieldComboBox();

  connect( mFieldsMappingTable, &QTableWidget::itemSelectionChanged, this, &QgsRelationAddPolymorphicDlg::updateFieldsMappingButtons );
  connect( mFieldsMappingAddButton, &QToolButton::clicked, this, &QgsRelationAddPolymorphicDlg::addFieldsRow );
  connect( mFieldsMappingRemoveButton, &QToolButton::clicked, this, &QgsRelationAddPolymorphicDlg::removeFieldsRow );
  connect( mReferencingLayerComboBox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddPolymorphicDlg::updateDialogButtons );
//  connect( mReferencedLayerExpressionWidget, &QgsFieldExpressionWidget::fieldChanged, this, &QgsRelationAddPolymorphicDlg::updateDialogButtons );
//  connect( mReferencedLayerFieldLayer, &QgsQ::layerChanged, this, &QgsRelationAddPolymorphicDlg::updateDialogButtons );
  connect( mReferencingLayerComboBox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddPolymorphicDlg::updateChildRelationsComboBox );
  connect( mReferencingLayerComboBox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddPolymorphicDlg::updateReferencingFieldsComboBoxes );
  connect( mReferencingLayerComboBox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddPolymorphicDlg::updateReferencedLayerFieldComboBox );
}

void QgsRelationAddPolymorphicDlg::updateTypeConfigWidget()
{
  updateDialogButtons();
}

void QgsRelationAddPolymorphicDlg::addFieldsRow()
{
  QgsFieldComboBox *referencingField = new QgsFieldComboBox( this );
  QLineEdit *referencedPolymorphicField = new QLineEdit( this );
  int index = mFieldsMappingTable->rowCount();

  referencingField->setLayer( mReferencingLayerComboBox->currentLayer() );

  mFieldsMappingTable->insertRow( index );
  mFieldsMappingTable->setCellWidget( index, 0, referencedPolymorphicField );
  mFieldsMappingTable->setCellWidget( index, 1, referencingField );

  updateFieldsMappingButtons();
  updateFieldsMappingHeaders();
  updateDialogButtons();
}

void QgsRelationAddPolymorphicDlg::removeFieldsRow()
{
  if ( mFieldsMappingTable->selectionModel()->hasSelection() )
  {
    for ( const QModelIndex &index : mFieldsMappingTable->selectionModel()->selectedRows() )
    {
      if ( index.row() == 0 )
        continue;

      if ( mFieldsMappingTable->rowCount() > 1 )
        mFieldsMappingTable->removeRow( index.row() );
    }
  }
  else
  {
    mFieldsMappingTable->removeRow( mFieldsMappingTable->rowCount() - 1 );
  }

  updateFieldsMappingButtons();
  updateFieldsMappingHeaders();
  updateDialogButtons();
}

void QgsRelationAddPolymorphicDlg::updateFieldsMappingButtons()
{
  int rowsCount = mFieldsMappingTable->rowCount();
  int selectedRowsCount = mFieldsMappingTable->selectionModel()->selectedRows().count();
  bool isRemoveButtonEnabled = selectedRowsCount <= rowsCount - 2;

  mFieldsMappingRemoveButton->setEnabled( isRemoveButtonEnabled );
}

void QgsRelationAddPolymorphicDlg::updateFieldsMappingHeaders()
{
  int rowsCount = mFieldsMappingTable->rowCount();
  QStringList verticalHeaderLabels;

  for ( int i = 0; i < rowsCount; i++ )
    verticalHeaderLabels << tr( "Field %1" ).arg( i + 1 );

  mFieldsMappingTable->setVerticalHeaderLabels( verticalHeaderLabels );
}

QString QgsRelationAddPolymorphicDlg::referencingLayerId()
{
  return mReferencingLayerComboBox->currentLayer()->id();
}

QString QgsRelationAddPolymorphicDlg::referencedLayerField()
{
  return mReferencedLayerFieldComboBox->currentField();
}

QString QgsRelationAddPolymorphicDlg::referencedLayerExpression()
{
  return mReferencedLayerExpressionWidget->expression();
}

QStringList QgsRelationAddPolymorphicDlg::referencedLayerIds()
{
  return QVariant( mReferencedLayersComboBox->checkedItemsData() ).toStringList();
}

QList< QPair< QString, QString > > QgsRelationAddPolymorphicDlg::references()
{
  QList< QPair< QString, QString > > references;
  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    QString referencedField = static_cast<QLineEdit *>( mFieldsMappingTable->cellWidget( i, 0 ) )->text();
    QString referencingField = static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 1 ) )->currentField();
    references << qMakePair( referencingField, referencedField );
  }

  return references;
}

QString QgsRelationAddPolymorphicDlg::relationId()
{
  return mIdLineEdit->text();
}

QString QgsRelationAddPolymorphicDlg::relationName()
{
  QgsVectorLayer *vl = static_cast<QgsVectorLayer *>( mReferencingLayerComboBox->currentLayer() );
  return tr( "Polymorphic relations for \"%1\"" ).arg( vl ? vl->name() : QStringLiteral( "<NO LAYER>" ) );
}

void QgsRelationAddPolymorphicDlg::updateDialogButtons()
{
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( isDefinitionValid() );
}

bool QgsRelationAddPolymorphicDlg::isDefinitionValid()
{
  bool isValid = true;
  return isValid;
  QgsMapLayer *referencedLayer = mReferencingLayerComboBox->currentLayer();
  isValid &= referencedLayer && referencedLayer->isValid();

  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    isValid &= !static_cast<QLineEdit *>( mFieldsMappingTable->cellWidget( i, 0 ) )->text().isNull();
    isValid &= !static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 1 ) )->currentField().isNull();
  }

  return isValid;
}

void QgsRelationAddPolymorphicDlg::updateChildRelationsComboBox()
{
  QgsVectorLayer *vl = static_cast<QgsVectorLayer *>( mReferencingLayerComboBox->currentLayer() );
  if ( !vl || !vl->isValid() )
    return;

  QStringList relationIdsList;

  const QList<QgsRelation> relations = QgsProject::instance()->relationManager()->referencedRelations( vl );
  for ( const QgsRelation &relation : relations )
  {
    if ( !relation.isValid() )
      continue;

    if ( relation.referencingLayer() != vl )
      continue;

    relationIdsList << relationName();
  }
}

void QgsRelationAddPolymorphicDlg::updateReferencingFieldsComboBoxes()
{
  QgsMapLayer *vl = mReferencingLayerComboBox->currentLayer();
  if ( !vl || !vl->isValid() )
    return;

  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    auto fieldComboBox = static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 1 ) );
    fieldComboBox->setLayer( vl );
  }
}

void QgsRelationAddPolymorphicDlg::updateReferencedLayerFieldComboBox()
{
  mReferencedLayerFieldComboBox->setLayer( mReferencingLayerComboBox->currentLayer() );
}
