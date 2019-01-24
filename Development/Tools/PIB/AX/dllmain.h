// dllmain.h : Declaration of module class.

class CATLUE3Module : public CAtlDllModuleT< CATLUE3Module >
{
public :
	DECLARE_LIBID(LIBID_ATLUE3Lib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ATLUE3, "{7A37D815-3BFD-4231-AE8F-75C97EC64C50}")
};

extern class CATLUE3Module _AtlModule;
