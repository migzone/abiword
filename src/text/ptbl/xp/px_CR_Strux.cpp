
#include "ut_types.h"
#include "px_ChangeRecord_Strux.h"
#include "px_ChangeRecord.h"

PX_ChangeRecord_Strux::PX_ChangeRecord_Strux(PXType type,
											 UT_Byte atomic,
											 PT_DocPosition position,
											 UT_Bool bLeftSide,
											 PT_AttrPropIndex indexOldAP,
											 PT_AttrPropIndex indexAP,
											 UT_Bool bTempBefore,
											 UT_Bool bTempAfter,
											 PTStruxType struxType)
	: PX_ChangeRecord(type, atomic, position, bLeftSide, indexOldAP, indexAP, bTempBefore, bTempAfter)
{
	m_struxType = struxType;
}

PX_ChangeRecord_Strux::~PX_ChangeRecord_Strux()
{
}

PX_ChangeRecord * PX_ChangeRecord_Strux::reverse(void) const
{
	PX_ChangeRecord_Strux * pcr
		= new PX_ChangeRecord_Strux(getRevType(),getRevFlags(),
									m_position,m_bLeftSide,
									m_indexAP,m_indexOldAP,
									m_bTempAfter,m_bTempBefore,
									m_struxType);
	UT_ASSERT(pcr);
	return pcr;
}

PTStruxType PX_ChangeRecord_Strux::getStruxType(void) const
{
	return m_struxType;
}

