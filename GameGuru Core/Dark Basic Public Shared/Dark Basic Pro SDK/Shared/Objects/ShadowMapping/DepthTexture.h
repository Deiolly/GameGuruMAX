#ifndef DEPTH_TEXTURE_H
#define DEPTH_TEXTURE_H

#include "preprocessor-flags.h"

#include "directx-macros.h"

//--------------------------------------------------------------------------------------
class DepthTexture
{
public:

	DepthTexture(const LPGG d3d);
	~DepthTexture();

	void			createTexture( LPGGDEVICE device, int width, int height );
	LPGGTEXTURE		getTexture ( void ) { return m_pTexture; }
	LPGGTEXTURE		m_pTexture;
	int				m_iWidth;
	int				m_iHeight;

	#ifdef DX11
	void*	getTextureView ( void ) { return m_pTextureView; }
	void*	m_pTextureView;
	void*	getTextureResourceView ( void ) { return m_pTextureResourceView; }
	void*	m_pTextureResourceView;
	#endif
};

#endif