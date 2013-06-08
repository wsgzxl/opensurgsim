#ifndef SURGSIM_MATH_MLCPCONSTRAINTTYPENAME_H
#define SURGSIM_MATH_MLCPCONSTRAINTTYPENAME_H

#include <string>
#include <SurgSim/Math/MlcpConstraintType.h>
#include <SurgSim/Framework/Assert.h>
#include <SurgSim/Framework/Log.h>

inline std::string getMlcpConstraintTypeName(MlcpConstraintType constraintType)
{
	switch (constraintType)
	{
	case MLCP_BILATERAL_1D_CONSTRAINT:
		return "MLCP_BILATERAL_1D_CONSTRAINT";
	case MLCP_BILATERAL_2D_CONSTRAINT:
		return "MLCP_BILATERAL_2D_CONSTRAINT";
	case MLCP_BILATERAL_3D_CONSTRAINT:
		return "MLCP_BILATERAL_3D_CONSTRAINT";
//	case MLCP_BILATERAL_4D_CONSTRAINT:
//		return "MLCP_BILATERAL_4D_CONSTRAINT";
	case MLCP_UNILATERAL_3D_FRICTIONLESS_CONSTRAINT:
		return "MLCP_UNILATERAL_3D_FRICTIONLESS_CONSTRAINT";
	case MLCP_UNILATERAL_3D_FRICTIONAL_CONSTRAINT:
		return "MLCP_UNILATERAL_3D_FRICTIONAL_CONSTRAINT";
	case MLCP_BILATERAL_FRICTIONLESS_SLIDING_CONSTRAINT:
		return "MLCP_BILATERAL_FRICTIONLESS_SLIDING_CONSTRAINT";
	case MLCP_BILATERAL_FRICTIONAL_SLIDING_CONSTRAINT:
		return "MLCP_BILATERAL_FRICTIONAL_SLIDING_CONSTRAINT";
	default:
		SURGSIM_LOG_SEVERE(SURGSIM_ASSERT_LOGGER) << "bad MLCP constraint type value: " << constraintType;
		return "";
	}
}

inline MlcpConstraintType getMlcpConstraintTypeValue(const std::string& typeName)
{

	if (typeName == "MLCP_BILATERAL_1D_CONSTRAINT")
	{
		return MLCP_BILATERAL_1D_CONSTRAINT;
	}
	else if (typeName == "MLCP_BILATERAL_2D_CONSTRAINT")
	{
		return MLCP_BILATERAL_2D_CONSTRAINT;
	}
	else if (typeName == "MLCP_BILATERAL_3D_CONSTRAINT")
	{
		return MLCP_BILATERAL_3D_CONSTRAINT;
	}
// 	else if (typeName == "MLCP_BILATERAL_4D_CONSTRAINT")
// 	{
// 		return MLCP_BILATERAL_4D_CONSTRAINT;
// 	}
	else if (typeName == "MLCP_UNILATERAL_3D_FRICTIONLESS_CONSTRAINT")
	{
		return MLCP_UNILATERAL_3D_FRICTIONLESS_CONSTRAINT;
	}
	else if (typeName == "MLCP_UNILATERAL_3D_FRICTIONAL_CONSTRAINT")
	{
		return MLCP_UNILATERAL_3D_FRICTIONAL_CONSTRAINT;
	}
	else if (typeName == "MLCP_BILATERAL_FRICTIONLESS_SLIDING_CONSTRAINT")
	{
		return MLCP_BILATERAL_FRICTIONLESS_SLIDING_CONSTRAINT;
	}
	else if (typeName == "MLCP_BILATERAL_FRICTIONAL_SLIDING_CONSTRAINT")
	{
		return MLCP_BILATERAL_FRICTIONAL_SLIDING_CONSTRAINT;
	}
	else
	{
		SURGSIM_LOG_WARNING(SURGSIM_ASSERT_LOGGER) << "bad MLCP constraint type name: '" << typeName << "'";
		return MLCP_INVALID_CONSTRAINT;
	}
}

#endif // SURGSIM_MATH_MLCPCONSTRAINTTYPENAME_H
