/***************************************************************************
    qgsrelationmanagerdialog.cpp
     --------------------------------------
    Date                 : 23.2.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdiscoverrelationsdialog.h"
#include "qgsrelationadddlg.h"
#include "qgsrelationaddpolymorphicdlg.h"
#include "qgsrelationmanagerdialog.h"
#include "qgsrelationmanager.h"
#include "qgspolymorphicrelation.h"
#include "qgsvectorlayer.h"

QgsRelationManagerDialog::QgsRelationManagerDialog( QgsRelationManager *relationMgr, QWidget *parent )
  : QWidget( parent )
  , Ui::QgsRelationManagerDialogBase()
  , mRelationManager( relationMgr )
{
  setupUi( this );

  mRelationsTree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

  connect( mBtnAddRelation, &QPushButton::clicked, this, &QgsRelationManagerDialog::mBtnAddRelation_clicked );
  connect( mActionAddPolymorphicRelation, &QAction::triggered, this, &QgsRelationManagerDialog::mActionAddPolymorphicRelation_triggered );
  connect( mBtnDiscoverRelations, &QPushButton::clicked, this, &QgsRelationManagerDialog::mBtnDiscoverRelations_clicked );
  connect( mBtnRemoveRelation, &QPushButton::clicked, this, &QgsRelationManagerDialog::mBtnRemoveRelation_clicked );

  mBtnRemoveRelation->setEnabled( false );
  mBtnAddRelation->setPopupMode( QToolButton::MenuButtonPopup );
  mBtnAddRelation->addAction( mActionAddPolymorphicRelation );

  connect( mRelationsTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QgsRelationManagerDialog::onSelectionChanged );
}

void QgsRelationManagerDialog::setLayers( const QList< QgsVectorLayer * > &layers )
{
  mLayers = layers;

  const QList<QgsRelation> &relations = mRelationManager->relations().values();

  const auto constRelations = relations;
  for ( const QgsRelation &rel : constRelations )
  {
    addRelation( rel );
  }

  mRelationsTree->sortByColumn( 0, Qt::AscendingOrder );
}

void QgsRelationManagerDialog::addRelation( const QgsRelation &rel )
{
  if ( ! rel.isValid() )
    return;

  if ( ! rel.polymorphicRelationId().isEmpty() )
    return;

  QString referencingFields = rel.fieldPairs().at( 0 ).referencingField();
  QString referencedFields = rel.fieldPairs().at( 0 ).referencedField();
  for ( int i = 1; i < rel.fieldPairs().count(); i++ )
  {
    referencingFields.append( QStringLiteral( ", %1" ).arg( rel.fieldPairs().at( i ).referencingField() ) );
    referencedFields.append( QStringLiteral( ", %1" ).arg( rel.fieldPairs().at( i ).referencedField() ) );
  }

  mRelationsTree->setSortingEnabled( false );
  int row = mRelationsTree->topLevelItemCount();
  QTreeWidgetItem *item = new QTreeWidgetItem();

  mRelationsTree->insertTopLevelItem( row, item );

  // Save relation in first column's item
  item->setData( 0, Qt::UserRole, QVariant::fromValue<QgsRelation>( rel ) );
  item->setFlags( Qt::ItemIsEnabled );

  item->setText( 0, rel.name() );
  item->setText( 1, rel.referencedLayer()->name() );
  item->setText( 2, referencedFields );
  item->setText( 3, rel.referencingLayer()->name() );
  item->setText( 4, referencingFields );
  item->setText( 5, rel.id() );
  item->setText( 6, rel.strength() == QgsRelation::RelationStrength::Composition
                 ? QStringLiteral( "Composition" )
                 : QStringLiteral( "Association" ) );

  mRelationsTree->setSortingEnabled( true );
}

void QgsRelationManagerDialog::addPolymorphicRelation( const QgsPolymorphicRelation &relation )
{
  if ( ! relation.isValid() )
    return;

  QString referencingFields;
  QString referencedFields;

  for ( int i = 0; i < relation.fieldPairs().count(); i++ )
  {
    if ( i != 0 )
    {
      referencingFields += QStringLiteral( ", " );
      referencedFields += QStringLiteral( ", " );
    }

    referencingFields += relation.fieldPairs().at( i ).referencingField();
    referencedFields += relation.fieldPairs().at( i ).referencedField();
  }

  mRelationsTree->setSortingEnabled( false );
  int row = mRelationsTree->topLevelItemCount();
  QTreeWidgetItem *item = new QTreeWidgetItem();

  mRelationsTree->insertTopLevelItem( row, item );

  // Save relation in first column's item
  item->setFlags( Qt::ItemIsEnabled );
  item->setData( 0, Qt::UserRole, QVariant::fromValue<QgsPolymorphicRelation>( relation ) );
  item->setText( 0, relation.name() );
  item->setText( 1, relation.referencingLayer()->name() );
  item->setText( 2, referencedFields );
  item->setText( 3, QStringLiteral( "as in \"%1\".\"%2\"" ).arg( relation.referencingLayer()->name(), relation.referencedLayerField() ) );
  item->setText( 4, referencingFields );
  item->setText( 5, relation.id() );

  const QList<QgsRelation> generatedRelations = relation.getGeneratedRelations();

  for ( const QgsRelation &generatedRelation : generatedRelations )
  {
    if ( !generatedRelation.isValid() )
      continue;

    QTreeWidgetItem *child = new QTreeWidgetItem();

    child->setFlags( Qt::ItemIsEnabled );
    child->setData( 0, Qt::UserRole, QVariant::fromValue<QgsRelation>( generatedRelation ) );
    child->setText( 0, generatedRelation.name() );
    child->setText( 1, generatedRelation.referencingLayer()->name() );
    child->setText( 2, referencedFields );
    child->setText( 3, generatedRelation.referencedLayer()->name() );
    child->setText( 4, referencingFields );
    child->setText( 5, generatedRelation.id() );
    child->setText( 6, generatedRelation.strength() == QgsRelation::RelationStrength::Composition
                    ? QStringLiteral( "Composition" )
                    : QStringLiteral( "Association" ) );

    item->addChild( child );
  }

  mRelationsTree->setSortingEnabled( true );
}

void QgsRelationManagerDialog::mBtnAddRelation_clicked()
{
  QgsRelationAddDlg addDlg;

  if ( addDlg.exec() )
  {
    QgsRelation relation;

    relation.setReferencingLayer( addDlg.referencingLayerId() );
    relation.setReferencedLayer( addDlg.referencedLayerId() );
    QString relationId = addDlg.relationId();
    if ( addDlg.relationId().isEmpty() )
      relationId = QStringLiteral( "%1_%2_%3_%4" )
                   .arg( addDlg.referencingLayerId().left( 10 ),
                         addDlg.references().at( 0 ).first,
                         addDlg.referencedLayerId().left( 10 ),
                         addDlg.references().at( 0 ).second );

    QStringList existingNames;

    const auto rels { relations() };
    for ( const QgsRelation &rel : rels )
    {
      existingNames << rel.id();
    }

    QString tempId = relationId + "_%1";
    int suffix = 1;
    while ( existingNames.contains( relationId ) )
    {
      relationId = tempId.arg( suffix );
      ++suffix;
    }
    relation.setId( relationId );
    const auto references = addDlg.references();
    for ( const auto &reference : references )
      relation.addFieldPair( reference.first, reference.second );
    relation.setName( addDlg.relationName() );
    relation.setStrength( addDlg.relationStrength() );

    addRelation( relation );
  }
}

void QgsRelationManagerDialog::mActionAddPolymorphicRelation_triggered()
{
  QgsRelationAddPolymorphicDlg addDlg;

  if ( addDlg.exec() )
  {
    QgsPolymorphicRelation relation;
    relation.setReferencingLayer( addDlg.referencingLayerId() );
    relation.setReferencedLayerField( addDlg.referencedLayerField() );
    relation.setReferencedLayerExpression( addDlg.referencedLayerExpression() );
    relation.setReferencedLayerIds( addDlg.referencedLayerIds() );

    const auto references = addDlg.references();
    for ( const auto &reference : references )
      relation.addFieldPair( reference.first, reference.second );

    QString relationId = addDlg.relationId();

    if ( relationId.isEmpty() )
      relation.generateId();
    else
      relation.setId( relationId );

    addPolymorphicRelation( relation );
  }
}

void QgsRelationManagerDialog::mBtnDiscoverRelations_clicked()
{
  QgsDiscoverRelationsDialog discoverDlg( relations(), mLayers, this );
  if ( discoverDlg.exec() )
  {
    const auto constRelations = discoverDlg.relations();
    for ( const QgsRelation &relation : constRelations )
    {
      addRelation( relation );
    }
  }
}

void QgsRelationManagerDialog::mBtnRemoveRelation_clicked()
{
  const QModelIndexList rows = mRelationsTree->selectionModel()->selectedRows();
  for ( int i = rows.size() - 1; i >= 0; --i )
  {
    mRelationsTree->takeTopLevelItem( rows[i].row() );
  }
}

QList< QgsRelation > QgsRelationManagerDialog::relations()
{
  QList< QgsRelation > relations;

//  int rows = mRelationsTree->rowCount();
//  relations.reserve( rows );
//  for ( int i = 0; i < rows; ++i )
//  {
//    QgsRelation relation = mRelationsTree->item( i, 0 )->data( Qt::UserRole ).value<QgsRelation>();
//    // The name can be edited in the table, so apply this one
//    relation.setName( mRelationsTree->item( i, 0 )->data( Qt::DisplayRole ).toString() );
//    relations << relation;
//  }

  return relations;
}

void QgsRelationManagerDialog::onSelectionChanged()
{
  mBtnRemoveRelation->setEnabled( mRelationsTree->selectionModel()->hasSelection() );
}
