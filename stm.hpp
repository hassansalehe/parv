#ifndef __STM_API_HPP__
#define __STM_API_HPP__

#include "support/defs.hpp"

#if defined (STM_LIB_RSTM)
#   include "rstm.hpp"
#   include "api/rstm_api.hpp"

#elif defined (STM_LIB_REDO_LOCK)
#   include "redo_lock.hpp"
#   include "api/redo_lock_api.hpp"

#elif defined (STM_LIB_LLT)
#   include "llt.hpp"
namespace stm { typedef LLTThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_ET)
#   include "et.hpp"
namespace stm { typedef ETThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_STM2)
#   include "stm2.hpp"
namespace stm { typedef AuxiliaryThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_STM2_HMT)
#   include "stm2_hmt.hpp"
namespace stm { typedef AuxiliaryThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_TML)
#   include "tml.hpp"
namespace stm { typedef TMLThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_TML_LAZY)
#   include "tml_lazy.hpp"
namespace stm { typedef TMLLazyThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_PRECISE)
#   include "precise.hpp"
namespace stm { typedef PreciseThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_FLOW)
#   include "flow.hpp"
namespace stm { typedef FlowThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_STRICT)
#   include "strict.hpp"
namespace stm { typedef StrictThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_SGLA)
#   include "sgla.hpp"
namespace stm { typedef SGLAThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_FAIR)
#   include "fair.hpp"
namespace stm { typedef FairThread Descriptor; }
#   include "api/wordbased_api.hpp"

#elif defined (STM_LIB_RINGSW)
#   include "ringsw.hpp"
namespace stm { typedef RingSWThread Descriptor; }
#   include "api/wordbased_api.hpp"

/** single lock, uninstrumented STM interface */
#elif defined (STM_LIB_CGL)
#   include "cgl.hpp"
namespace stm { typedef CGLThread Descriptor; }
#   include "api/cgl_api.hpp"

#else
#error "Error, no version defined"
#endif

#endif // __STM_API_HPP__
