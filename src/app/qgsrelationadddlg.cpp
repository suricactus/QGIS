/***************************************************************************
  qgsrelationadddlg.cpp - QgsRelationAddDlg
  ---------------------------------

 begin                : 4.10.2013
 copyright            : (C) 2013 by Matthias Kuhn
 email                : matthias@opengis.ch
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

#include "qgsrelationadddlg.h"
#include "qgsvectorlayer.h"
#include "qgsmaplayercombobox.h"
#include "qgsfieldcombobox.h"
#include "qgsmaplayerproxymodel.h"
#include "qgsapplication.h"
#include "qgshelp.h"
#include "qgsproject.h"
#include "qgsrelationmanager.h"



QgsFieldPairWidget::QgsFieldPairWidget( int index, QWidget *parent )
  : QWidget( parent )
  , mIndex( index )
  , mEnabled( index == 0 )
{
  mLayout = new QHBoxLayout();
  mLayout->setContentsMargins( 0, 0, 0, 0 );

  mAddButton = new QToolButton( this );
  mAddButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/symbologyAdd.svg" ) ) );
  mAddButton->setMinimumWidth( 30 );
  mAddButton->setToolTip( "Add new field pair as part of a composite foreign key" );
  mLayout->addWidget( mAddButton, 0, Qt::AlignLeft );

  mRemoveButton = new QToolButton( this );
  mRemoveButton->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/symbologyRemove.svg" ) ) );
  mRemoveButton->setMinimumWidth( 30 );
  mRemoveButton->setToolTip( "Remove the last pair of fields" );
  mLayout->addWidget( mRemoveButton, 0, Qt::AlignLeft );

  mSpacerItem = new QSpacerItem( 30, 30, QSizePolicy::Minimum, QSizePolicy::Maximum );
  mLayout->addSpacerItem( mSpacerItem );

  mReferencedFieldCombobox = new QgsFieldComboBox( this );
  connect( mReferencedFieldCombobox, &QgsFieldComboBox::fieldChanged, this, &QgsFieldPairWidget::configChanged );
  mLayout->addWidget( mReferencedFieldCombobox, 1 );

  mReferencingFieldCombobox = new QgsFieldComboBox( this );
  connect( mReferencingFieldCombobox, &QgsFieldComboBox::fieldChanged, this, &QgsFieldPairWidget::configChanged );
  mLayout->addWidget( mReferencingFieldCombobox, 1 );

  setLayout( mLayout );
  updateWidgetVisibility();

  connect( mAddButton, &QToolButton::clicked, this, &QgsFieldPairWidget::changeEnable );
  connect( mRemoveButton, &QToolButton::clicked, this, &QgsFieldPairWidget::changeEnable );
  connect( mAddButton, &QToolButton::clicked, this, &QgsFieldPairWidget::pairEnabled );
  connect( mRemoveButton, &QToolButton::clicked, this, &QgsFieldPairWidget::pairDisabled );
}

void QgsFieldPairWidget::setReferencingLayer( QgsMapLayer *layer )
{
  mReferencingFieldCombobox->setLayer( layer );
}

void QgsFieldPairWidget::setReferencedLayer( QgsMapLayer *layer )
{
  mReferencedFieldCombobox->setLayer( layer );
}

QString QgsFieldPairWidget::referencingField() const
{
  return mReferencingFieldCombobox->currentField();
}

QString QgsFieldPairWidget::referencedField() const
{
  return mReferencedFieldCombobox->currentField();
}

bool QgsFieldPairWidget::isPairEnabled() const
{
  return mEnabled;
}

void QgsFieldPairWidget::updateWidgetVisibility()
{
  mAddButton->setVisible( !mEnabled );
  mRemoveButton->setVisible( mIndex > 0 && mEnabled );
  mReferencingFieldCombobox->setVisible( mEnabled );
  mReferencedFieldCombobox->setVisible( mEnabled );
  int spacerSize = 0;
  if ( !mRemoveButton->isVisible() && !mAddButton->isVisible() )
    spacerSize = mRemoveButton->minimumWidth() + mLayout->spacing();
  mSpacerItem->changeSize( spacerSize, spacerSize );
}

void QgsFieldPairWidget::changeEnable()
{
  mEnabled = !mEnabled || ( mIndex == 0 );
  updateWidgetVisibility();
  emit configChanged();
}

////////////////////
// QgsRelationAddDlg

QgsRelationAddDlg::QgsRelationAddDlg( QWidget *parent )
  : QDialog( parent )
  , Ui::QgsRelationManagerAddDialogBase()
{
  setupUi( this );
  setWindowTitle( tr( "Add New Relation" ) );

  mReferencedLayerCombobox = new QgsMapLayerComboBox( this );
  mReferencedLayerCombobox->setFilters( QgsMapLayerProxyModel::VectorLayer );
  mFieldsMappingTable->setCellWidget( 0, 0, mReferencedLayerCombobox );

  mReferencingLayerCombobox = new QgsMapLayerComboBox( this );
  mReferencingLayerCombobox->setFilters( QgsMapLayerProxyModel::VectorLayer );
  mFieldsMappingTable->setCellWidget( 0, 1, mReferencingLayerCombobox );

  mRelationStrengthComboBox->addItem( "Association", QVariant::fromValue( QgsRelation::RelationStrength::Association ) );
  mRelationStrengthComboBox->addItem( "Composition", QVariant::fromValue( QgsRelation::RelationStrength::Composition ) );

  mRelationStrengthComboBox->addItem( tr( "Association" ), QVariant::fromValue( QgsRelation::RelationStrength::Association ) );
  mRelationStrengthComboBox->addItem( tr( "Composition" ), QVariant::fromValue( QgsRelation::RelationStrength::Composition ) );
  mRelationStrengthComboBox->setToolTip( tr( "When composition is selected the child features will be duplicated too.\n"
                                         "Duplications are made by the feature duplication action.\n"
                                         "The default actions are activated in the Action section of the layer properties." ) );

  mButtonBox->setStandardButtons( QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::Ok );
  connect( mButtonBox, &QDialogButtonBox::accepted, this, &QgsRelationAddDlg::accept );
  connect( mButtonBox, &QDialogButtonBox::rejected, this, &QgsRelationAddDlg::reject );
  connect( mButtonBox, &QDialogButtonBox::helpRequested, this, [ = ]
  {
    QgsHelp::openHelp( QStringLiteral( "working_with_vector/attribute_table.html#defining-1-n-relations" ) );
  } );


  addFieldsRow();
  updateTypeConfigWidget();
  updateDialogButtons();

  connect( mNormalTypeRadioButton, &QRadioButton::clicked, this, &QgsRelationAddDlg::updateTypeConfigWidget );
  connect( mDynamicTypeRadioButton, &QRadioButton::clicked, this, &QgsRelationAddDlg::updateTypeConfigWidget );
  connect( mFieldsMappingTable, &QTableWidget::itemSelectionChanged, this, &QgsRelationAddDlg::updateFieldsMappingButtons );
  connect( mFieldsMappingAddButton, &QToolButton::clicked, this, &QgsRelationAddDlg::addFieldsRow );
  connect( mFieldsMappingRemoveButton, &QToolButton::clicked, this, &QgsRelationAddDlg::removeFieldsRow );
  connect( mReferencingLayerCombobox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddDlg::updateDialogButtons );
  connect( mReferencedLayerCombobox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddDlg::updateDialogButtons );
  connect( mReferencedLayerCombobox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddDlg::updateReferencedFieldsComboBoxes );
  connect( mReferencingLayerCombobox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddDlg::updateChildRelationsComboBox );
  connect( mReferencingLayerCombobox, &QgsMapLayerComboBox::layerChanged, this, &QgsRelationAddDlg::updateReferencingFieldsComboBoxes );
}

void QgsRelationAddDlg::updateTypeConfigWidget()
{
  switch ( type() )
  {
    case QgsRelation::Normal:
      mTypeStackedWidget->setCurrentWidget( mNormalTypeWidget );
      mFieldsMappingTable->showColumn( 1 );
      break;
    case QgsRelation::Dynamic:
      mTypeStackedWidget->setCurrentWidget( mDynamicTypeWidget );
      mFieldsMappingTable->hideColumn( 1 );
      updateChildRelationsComboBox();
      break;
  }

  updateDialogButtons();
}

QgsRelation::RelationType QgsRelationAddDlg::type()
{
  if ( mDynamicTypeRadioButton->isChecked() )
    return QgsRelation::Dynamic;
  else
    return QgsRelation::Normal;
}

void QgsRelationAddDlg::addFieldsRow()
{
  QgsFieldComboBox *referencedField = new QgsFieldComboBox( this );
  QgsFieldComboBox *referencingField = new QgsFieldComboBox( this );
  int index = mFieldsMappingTable->rowCount();

  referencedField->setLayer( mReferencedLayerCombobox->currentLayer() );
  referencingField->setLayer( mReferencingLayerCombobox->currentLayer() );

  mFieldsMappingTable->insertRow( index );
  mFieldsMappingTable->setCellWidget( index, 0, referencedField );
  mFieldsMappingTable->setCellWidget( index, 1, referencingField );

  updateFieldsMappingButtons();
  updateFieldsMappingHeaders();
  updateDialogButtons();
}

void QgsRelationAddDlg::removeFieldsRow()
{
  if ( mFieldsMappingTable->selectionModel()->hasSelection() )
  {
    for ( const QModelIndex &index : mFieldsMappingTable->selectionModel()->selectedRows() )
    {
      if ( index.row() == 0 )
        continue;

      if ( mFieldsMappingTable->rowCount() > 2 )
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

void QgsRelationAddDlg::updateFieldsMappingButtons()
{
  int rowsCount = mFieldsMappingTable->rowCount();
  int selectedRowsCount = mFieldsMappingTable->selectionModel()->selectedRows().count();
  bool isLayersRowSelected = mFieldsMappingTable->selectionModel()->isRowSelected( 0, QModelIndex() );
  bool isRemoveButtonEnabled = !isLayersRowSelected && selectedRowsCount <= rowsCount - 2;

  mFieldsMappingRemoveButton->setEnabled( isRemoveButtonEnabled );
}

void QgsRelationAddDlg::updateFieldsMappingHeaders()
{
  int rowsCount = mFieldsMappingTable->rowCount();
  QStringList verticalHeaderLabels( {tr( "Layer" )} );

  for ( int i = 0; i < rowsCount; i++ )
    verticalHeaderLabels << tr( "Field %1" ).arg( i + 1 );

  mFieldsMappingTable->setVerticalHeaderLabels( verticalHeaderLabels );
}

QString QgsRelationAddDlg::referencingLayerId()
{
  return mReferencingLayerCombobox->currentLayer()->id();
}

QString QgsRelationAddDlg::referencedLayerId()
{
  return mReferencedLayerCombobox->currentLayer()->id();
}

QList< QPair< QString, QString > > QgsRelationAddDlg::references()
{
  // QList< QPair< QString, QString > > references;
  // for ( int i = 0; i < mFieldPairWidgets.count(); i++ )
  // {
  //   if ( !mFieldPairWidgets.at( i )->isPairEnabled() )
  //     continue;
  //   QString referencingField = mFieldPairWidgets.at( i )->referencingField();
  //   QString referencedField = mFieldPairWidgets.at( i )->referencedField();
  //   references << qMakePair( referencingField, referencedField );
  // }
  // return references;
}

QString QgsRelationAddDlg::relationId()
{
  return mIdLineEdit->text();
}

QString QgsRelationAddDlg::relationName()
{
  return mNameLineEdit->text();
}

QgsRelation::RelationStrength QgsRelationAddDlg::relationStrength()
{
  return mRelationStrengthComboBox->currentData().value<QgsRelation::RelationStrength>();
}

void QgsRelationAddDlg::updateDialogButtons()
{
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( isDefinitionValid() );
}

bool QgsRelationAddDlg::isDefinitionValid()
{
  bool isValid = true;
  QgsMapLayer *referencedLayer = mReferencedLayerCombobox->currentLayer();
  isValid &= referencedLayer && referencedLayer->isValid();

  if ( type() == QgsRelation::Normal )
  {
    QgsMapLayer *referencingLayer = mReferencingLayerCombobox->currentLayer();
    isValid &= referencingLayer && referencingLayer->isValid();
  }

  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    if ( i == 0 )
      continue;

    isValid &= !static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 0 ) )->currentField().isNull();

    if ( type() == QgsRelation::Normal )
      isValid &= !static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 1 ) )->currentField().isNull();
  }

  return isValid;
}

void QgsRelationAddDlg::updateChildRelationsComboBox()
{
  mChildRelationsComboBox->clear();

  QgsVectorLayer *vl = static_cast<QgsVectorLayer *>( mReferencedLayerCombobox->currentLayer() );
  if ( !vl || !vl->isValid() )
    return;

  const QList<QgsRelation> relations = QgsProject::instance()->relationManager()->referencedRelations( vl );
  for ( const QgsRelation &relation : relations )
  {
    if ( !relation.isValid() || relation.type() == QgsRelation::Dynamic )
      continue;

    mChildRelationsComboBox->addItem( relation.name(), relation.id() );
  }
}

void QgsRelationAddDlg::updateReferencedFieldsComboBoxes()
{
  QgsMapLayer *vl = mReferencedLayerCombobox->currentLayer();
  if ( !vl || !vl->isValid() )
    return;

  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    if ( i == 0 )
      continue;

    auto fieldComboBox = static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 0 ) );
    fieldComboBox->setLayer( vl );
  }
}

void QgsRelationAddDlg::updateReferencingFieldsComboBoxes()
{
  QgsMapLayer *vl = mReferencingLayerCombobox->currentLayer();
  if ( !vl || !vl->isValid() )
    return;

  for ( int i = 0, l = mFieldsMappingTable->rowCount(); i < l; i++ )
  {
    if ( i == 0 )
      continue;

    auto fieldComboBox = static_cast<QgsFieldComboBox *>( mFieldsMappingTable->cellWidget( i, 1 ) );
    fieldComboBox->setLayer( vl );
  }
}
