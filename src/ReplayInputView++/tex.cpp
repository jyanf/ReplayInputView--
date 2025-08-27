#include "tex.hpp"

namespace riv::tex{

	int create_texture_byid(int offset) {
		int id;
		auto pphandle = SokuLib::textureMgr.allocate(&id);
		if (pphandle)
			*pphandle = NULL;
		if (SUCCEEDED(D3DXCreateTextureFromResource(SokuLib::pd3dDev, hDllModule, MAKEINTRESOURCE(offset), pphandle))) {
			return id;
		}
		else {
			SokuLib::textureMgr.deallocate(id);
			return 0;
		}
	}

}
