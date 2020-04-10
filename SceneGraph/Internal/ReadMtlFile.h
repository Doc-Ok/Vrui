/***********************************************************************
ReadMtlFile - Helper function to read a material library file in
Wavefront OBJ format into a material library node.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_INTERNAL_READMRLFILE_INCLUDED
#define SCENEGRAPH_INTERNAL_READMTLFILE_INCLUDED

#include <string>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace SceneGraph {
class MaterialLibraryNode;
}

namespace SceneGraph {

void readMtlFile(IO::Directory& directory,const std::string& fileName,MaterialLibraryNode& materialLibrary,bool disableTextures); // Reads the Wavefront OBJ material library file of the given name from the given directory and appends read materials to the given material library node; ignores texture images if disableTextures is true

}

#endif
