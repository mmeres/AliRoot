#ifndef ALIEMCALTRIGGERTRUDCSCONFIG_H
#define ALIEMCALTRIGGERTRUDCSCONFIG_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

//________________________________________________
/// \class AliEMCALTriggerTRUDCSConfig
/// \ingroup EMCALbase
/// \brief TRU DCS Config
///
/// Add comment
///
/// \author: R. GUERNANE LPSC Grenoble CNRS/IN2P3
/// \author: Jiri Kral, JYU
//________________________________________________

#include "TObject.h"
#include <iosfwd>

class AliEMCALTriggerTRUDCSConfig : public TObject 
{

public:

	AliEMCALTriggerTRUDCSConfig();
	virtual ~AliEMCALTriggerTRUDCSConfig() {}

	/**
 	 * @brief equalty operator
   * 
	 * Checking if the two TRU DCS configurations are equal. For equalty
	 * all settings must be the same.
	 */ 
	bool operator==(const AliEMCALTriggerTRUDCSConfig &other) const;

	/**
	 * @brief Streaming operator
	 * 
	 * Printing all settings of the given TRU on the output stream
	 */
	friend std::ostream &operator<<(std::ostream &stream, const AliEMCALTriggerTRUDCSConfig &other);

	/**
	 * @brief Serialize object to JSON format
	 * 
	 * @return JSON-serialized TRU DCS config object 
	 */
	std::string ToJSON() const;

	void    SetSELPF(  UInt_t pf)              { fSELPF  = pf;        }
	void    SetL0SEL(  UInt_t la)              { fL0SEL  = la;        }
	void    SetL0COSM( UInt_t lc)              { fL0COSM = lc;        }
	void    SetGTHRL0( UInt_t lg)              { fGTHRL0 = lg;        }
	void    SetMaskReg(UInt_t msk, Int_t pos)  { fMaskReg[pos] = msk; }
	void    SetRLBKSTU(UInt_t rb)              { fRLBKSTU = rb;       }
	void    SetFw(     UInt_t fw)              { fFw = fw;            }
			
	UInt_t  GetSELPF()                   const { return fSELPF;       }
	UInt_t  GetL0SEL()                   const { return fL0SEL;       }
	UInt_t  GetL0COSM()                  const { return fL0COSM;      }
	UInt_t  GetGTHRL0()                  const { return fGTHRL0;      }
	UInt_t  GetMaskReg(Int_t pos)        const { return fMaskReg[pos];}
	UInt_t  GetRLBKSTU()                 const { return fRLBKSTU;     }
	UInt_t  GetFw()                      const { return fFw;          }
	
	Int_t   GetSegmentation();
	
protected:

	AliEMCALTriggerTRUDCSConfig           (const AliEMCALTriggerTRUDCSConfig &cd);
	AliEMCALTriggerTRUDCSConfig &operator=(const AliEMCALTriggerTRUDCSConfig &cd);

private:
	
  UInt_t   fSELPF;                         ///< PeakFinder setup
  UInt_t   fL0SEL;                         ///< L0 Algo selection
  UInt_t   fL0COSM;                        ///< 2x2
  UInt_t   fGTHRL0;                        ///< 4x4
  UInt_t   fMaskReg[6];                    ///< 6*16 = 96 mask bits per TRU
  UInt_t   fRLBKSTU;                       ///< TRU circular buffer rollback
  UInt_t   fFw;                            ///< TRU fw version
	
  /// \cond CLASSIMP
  ClassDef(AliEMCALTriggerTRUDCSConfig,4) ;
  /// \endcond

};

#endif // ALIEMCALTRIGGERTRUDCSCONFIG_H
