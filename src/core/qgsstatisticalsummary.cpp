/***************************************************************************
  qgsstatisticalsummary.cpp
  --------------------------------------
  Date                 : May 2015
  Copyright            : (C) 2015 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsstatisticalsummary.h"
#include <limits>
#include <QString>
#include <QObject>

/***************************************************************************
 * This class is considered CRITICAL and any change MUST be accompanied with
 * full unit tests in testqgsstatisticalsummary.cpp.
 * See details in QEP #17
 ****************************************************************************/

QgsStatisticalSummary::QgsStatisticalSummary( Statistics stats )
  : mStatistics( stats )
{
  reset();
}

void QgsStatisticalSummary::setStatistics( QgsStatisticalSummary::Statistics stats )
{
  mStatistics = stats;
  reset();
}

void QgsStatisticalSummary::reset()
{
  mFirst = std::numeric_limits<double>::quiet_NaN();
  mLast = std::numeric_limits<double>::quiet_NaN();
  mCount = 0;
  mMissing = 0;
  mSum = 0;
  mMean = 0;
  mMedian = 0;
  mMin = std::numeric_limits<double>::max();
  mMax = -std::numeric_limits<double>::max();
  mStdev = 0;
  mSampleStdev = 0;
  mMinority = 0;
  mMajority = 0;
  mFirstQuartile = 0;
  mThirdQuartile = 0;
  mValueCount.clear();
  mValues.clear();

  mRequiresHisto = mStatistics & QgsStatisticalSummary::Majority || mStatistics & QgsStatisticalSummary::Minority ||
                   mStatistics & QgsStatisticalSummary::Variety || mStatistics & QgsStatisticalSummary::Mode;

  mRequiresAllValueStorage = mStatistics & QgsStatisticalSummary::StDev || mStatistics & QgsStatisticalSummary::StDevSample ||
                             mStatistics & QgsStatisticalSummary::Median || mStatistics & QgsStatisticalSummary::FirstQuartile ||
                             mStatistics & QgsStatisticalSummary::ThirdQuartile || mStatistics & QgsStatisticalSummary::InterQuartileRange;
}

/***************************************************************************
 * This class is considered CRITICAL and any change MUST be accompanied with
 * full unit tests in testqgsstatisticalsummary.cpp.
 * See details in QEP #17
 ****************************************************************************/

void QgsStatisticalSummary::calculate( const QList<double> &values )
{
  reset();

  for ( double value : values )
  {
    addValue( value );
  }

  finalize();
}

void QgsStatisticalSummary::addValue( double value )
{
  if ( mCount == 0 )
    mFirst = value;
  mCount++;
  mSum += value;
  mMin = std::min( mMin, value );
  mMax = std::max( mMax, value );
  mLast = value;

  if ( mRequiresHisto )
    mValueCount.insert( value, mValueCount.value( value, 0 ) + 1 );

  if ( mRequiresAllValueStorage )
    mValues << value;
}

void QgsStatisticalSummary::addVariant( const QVariant &value )
{
  bool convertOk = false;
  if ( !value.isValid() || value.isNull() )
    mMissing++;
  else
  {
    double val = value.toDouble( &convertOk );
    if ( convertOk )
      addValue( val );
    else
      mMissing++;
  }
}

void QgsStatisticalSummary::finalize()
{
  if ( mCount == 0 )
  {
    mFirst = std::numeric_limits<double>::quiet_NaN();
    mLast = std::numeric_limits<double>::quiet_NaN();
    mMin = std::numeric_limits<double>::quiet_NaN();
    mMax = std::numeric_limits<double>::quiet_NaN();
    mMean = std::numeric_limits<double>::quiet_NaN();
    mMedian = std::numeric_limits<double>::quiet_NaN();
    mStdev = std::numeric_limits<double>::quiet_NaN();
    mSampleStdev = std::numeric_limits<double>::quiet_NaN();
    mMinority = std::numeric_limits<double>::quiet_NaN();
    mMajority = std::numeric_limits<double>::quiet_NaN();
    mFirstQuartile = std::numeric_limits<double>::quiet_NaN();
    mThirdQuartile = std::numeric_limits<double>::quiet_NaN();
    mMode.empty();
    return;
  }

  mMean = mSum / mCount;

  if ( mStatistics & QgsStatisticalSummary::StDev || mStatistics & QgsStatisticalSummary::StDevSample )
  {
    double sumSquared = 0;
    const auto constMValues = mValues;
    for ( double value : constMValues )
    {
      double diff = value - mMean;
      sumSquared += diff * diff;
    }
    mStdev = std::pow( sumSquared / mValues.count(), 0.5 );
    mSampleStdev = std::pow( sumSquared / ( mValues.count() - 1 ), 0.5 );
  }

  if ( mStatistics & QgsStatisticalSummary::Median
       || mStatistics & QgsStatisticalSummary::FirstQuartile
       || mStatistics & QgsStatisticalSummary::ThirdQuartile
       || mStatistics & QgsStatisticalSummary::InterQuartileRange )
  {
    std::sort( mValues.begin(), mValues.end() );
    bool even = ( mCount % 2 ) < 1;
    if ( even )
    {
      mMedian = ( mValues[mCount / 2 - 1] + mValues[mCount / 2] ) / 2.0;
    }
    else //odd
    {
      mMedian = mValues[( mCount + 1 ) / 2 - 1];
    }
  }

  if ( mStatistics & QgsStatisticalSummary::FirstQuartile
       || mStatistics & QgsStatisticalSummary::InterQuartileRange )
  {
    if ( ( mCount % 2 ) < 1 )
    {
      int halfCount = mCount / 2;
      bool even = ( halfCount % 2 ) < 1;
      if ( even )
      {
        mFirstQuartile = ( mValues[halfCount / 2 - 1] + mValues[halfCount / 2] ) / 2.0;
      }
      else //odd
      {
        mFirstQuartile = mValues[( halfCount  + 1 ) / 2 - 1];
      }
    }
    else
    {
      int halfCount = mCount / 2 + 1;
      bool even = ( halfCount % 2 ) < 1;
      if ( even )
      {
        mFirstQuartile = ( mValues[halfCount / 2 - 1] + mValues[halfCount / 2] ) / 2.0;
      }
      else //odd
      {
        mFirstQuartile = mValues[( halfCount  + 1 ) / 2 - 1];
      }
    }
  }

  if ( mStatistics & QgsStatisticalSummary::ThirdQuartile
       || mStatistics & QgsStatisticalSummary::InterQuartileRange )
  {
    if ( ( mCount % 2 ) < 1 )
    {
      int halfCount = mCount / 2;
      bool even = ( halfCount % 2 ) < 1;
      if ( even )
      {
        mThirdQuartile = ( mValues[ halfCount + halfCount / 2 - 1] + mValues[ halfCount + halfCount / 2] ) / 2.0;
      }
      else //odd
      {
        mThirdQuartile = mValues[( halfCount + 1 ) / 2 - 1 + halfCount ];
      }
    }
    else
    {
      int halfCount = mCount / 2 + 1;
      bool even = ( halfCount % 2 ) < 1;
      if ( even )
      {
        mThirdQuartile = ( mValues[ halfCount + halfCount / 2 - 2 ] + mValues[ halfCount + halfCount / 2 - 1 ] ) / 2.0;
      }
      else //odd
      {
        mThirdQuartile = mValues[( halfCount + 1 ) / 2 - 2 + halfCount ];
      }
    }
  }

  if ( mStatistics & QgsStatisticalSummary::Minority || mStatistics & QgsStatisticalSummary::Majority )
  {
    QList<int> valueCounts = mValueCount.values();

    if ( mStatistics & QgsStatisticalSummary::Minority )
    {
      mMinority = mValueCount.key( *std::min_element( valueCounts.begin(), valueCounts.end() ) );
    }
    if ( mStatistics & QgsStatisticalSummary::Majority )
    {
      mMajority = mValueCount.key( *std::max_element( valueCounts.begin(), valueCounts.end() ) );
    }
  }

  if ( mStatistics & QgsStatisticalSummary::Mode )
  {
    int maxOccurrences = 0;

    for ( const double key : mValueCount.keys() )
    {
      int occurrences = mValueCount[key];

      if ( occurrences > maxOccurrences )
      {
        mMode.clear();
        maxOccurrences = occurrences;
      }
      else if ( occurrences == 0 || occurrences < maxOccurrences )
      {
        continue;
      }

      mMode.append( key );
    }
  }

}

/***************************************************************************
 * This class is considered CRITICAL and any change MUST be accompanied with
 * full unit tests in testqgsstatisticalsummary.cpp.
 * See details in QEP #17
 ****************************************************************************/

double QgsStatisticalSummary::statistic( QgsStatisticalSummary::Statistic stat ) const
{
  switch ( stat )
  {
    case Count:
      return mCount;
    case CountMissing:
      return mMissing;
    case Sum:
      return mSum;
    case Mean:
      return mMean;
    case Median:
      return mMedian;
    case StDev:
      return mStdev;
    case StDevSample:
      return mSampleStdev;
    case Min:
      return mMin;
    case Max:
      return mMax;
    case Range:
      return mMax - mMin;
    case Minority:
      return mMinority;
    case Majority:
      return mMajority;
    case Variety:
      return mValueCount.count();
    case FirstQuartile:
      return mFirstQuartile;
    case ThirdQuartile:
      return mThirdQuartile;
    case InterQuartileRange:
      return mThirdQuartile - mFirstQuartile;
    case First:
      return mFirst;
    case Last:
      return mLast;
    case Mode:
      // return NaN as we have to return a list of numbers, but the function retuns a single number
      // this statistic can be accessed only with explicit call of the `mode()` method
      return std::numeric_limits<double>::quiet_NaN();
    case All:
      return 0;
  }
  return 0;
}

QString QgsStatisticalSummary::displayName( QgsStatisticalSummary::Statistic statistic )
{
  switch ( statistic )
  {
    case Count:
      return QObject::tr( "Count" );
    case CountMissing:
      return QObject::tr( "Count (missing)" );
    case Sum:
      return QObject::tr( "Sum" );
    case Mean:
      return QObject::tr( "Mean" );
    case Median:
      return QObject::tr( "Median" );
    case StDev:
      return QObject::tr( "St dev (pop)" );
    case StDevSample:
      return QObject::tr( "St dev (sample)" );
    case Min:
      return QObject::tr( "Minimum" );
    case Max:
      return QObject::tr( "Maximum" );
    case Range:
      return QObject::tr( "Range" );
    case Minority:
      return QObject::tr( "Minority" );
    case Majority:
      return QObject::tr( "Majority" );
    case Variety:
      return QObject::tr( "Variety" );
    case FirstQuartile:
      return QObject::tr( "Q1" );
    case ThirdQuartile:
      return QObject::tr( "Q3" );
    case InterQuartileRange:
      return QObject::tr( "IQR" );
    case First:
      return QObject::tr( "First" );
    case Last:
      return QObject::tr( "Last" );
    case Mode:
      return QObject::tr( "Mode" );
    case All:
      return QString();
  }
  return QString();
}

QString QgsStatisticalSummary::shortName( QgsStatisticalSummary::Statistic statistic )
{
  switch ( statistic )
  {
    case Count:
      return QStringLiteral( "count" );
    case CountMissing:
      return QStringLiteral( "countmissing" );
    case Sum:
      return QStringLiteral( "sum" );
    case Mean:
      return QStringLiteral( "mean" );
    case Median:
      return QStringLiteral( "median" );
    case StDev:
      return QStringLiteral( "stdev" );
    case StDevSample:
      return QStringLiteral( "stdevsample" );
    case Min:
      return QStringLiteral( "min" );
    case Max:
      return QStringLiteral( "max" );
    case Range:
      return QStringLiteral( "range" );
    case Minority:
      return QStringLiteral( "minority" );
    case Majority:
      return QStringLiteral( "majority" );
    case Variety:
      return QStringLiteral( "variety" );
    case FirstQuartile:
      return QStringLiteral( "q1" );
    case ThirdQuartile:
      return QStringLiteral( "q3" );
    case InterQuartileRange:
      return QStringLiteral( "iqr" );
    case First:
      return QStringLiteral( "first" );
    case Last:
      return QStringLiteral( "last" );
    case Mode:
      return QStringLiteral( "mode" );
    case All:
      return QString();
  }
  return QString();
}

