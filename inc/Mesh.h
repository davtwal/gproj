// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Mesh.h
// * Copyright (C) DigiPen Institute of Technology 2019
// * 
// * Created     : 2019y 09m 25d
// * Last Altered: 2019y 09m 25d
// * 
// * Author      : David Walker
// * E-mail      : d.walker\@digipen.edu
// * 
// * Description :
// *
// *
// *
// *
// * 
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#ifndef DW_MESH_H
#define DW_MESH_H

#include "Utils.h"
#include "Buffer.h"
#include "Vertex.h"

namespace dw {
  class Queue;
  class CommandBuffer;

  class Mesh {
  public:
    Mesh(std::vector<Vertex> vertices = {}, std::vector<uint32_t> indices = {});
    ~Mesh();

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    Mesh(Mesh const&)            = delete;
    Mesh& operator=(Mesh const&) = delete;

    NO_DISCARD Buffer& getVertexBuffer();
    NO_DISCARD Buffer& getIndexBuffer();

    NO_DISCARD size_t getNumVertices() const;
    NO_DISCARD size_t getNumIndices() const;

    NO_DISCARD size_t getVertexSize() const;
    NO_DISCARD size_t getIndexSize() const;

    // returns the size of all of the vertices
    // e.g. numVertices * sizeof Vertex
    NO_DISCARD size_t getSizeOfVertices() const;
    NO_DISCARD size_t getSizeOfIndices() const;

    // TODO: These can be better.
    // I'd like to be able to have two functions overall:
    // upload(cmdBuff, queue) -> uploads & submits everything.
    // upload(cmdBuff) -> uploads to staging buffers, adds commands to the command buffer,
    //                    but doesn't submit to a queue. this is for adding more cmds to the
    //                    queue before submitting to create bigger batches
    //void uploadStaging(Buffer& vertStage, Buffer& indexStage);

    // returns the staging buffers that are used. these staging buffers should be kept alive
    // until cmdBuff has finished executing on the queue. assumes cmdBuff has been started already

    /*  UPLOAD [CmdBuff, Queue] -> All in one. Uploads & submits queue
     *  UPLOAD [CmdBuff]        -> Uploads but no submit. Returns staging buffers.
     *
     *  CREATE BUFFERS -> Creates vertex/index buffers.
     *  CREATE STAGING -> Creates staging buffers and returns them.
     *  UPLOAD STAGING -> Uploads the vertex data onto the staging buffers.
     *  UPLOAD COMMANDS -> Puts the copy commands onto the command buffer.
     */

    // vertex, index
    using StagingBuffs = std::pair<Buffer, Buffer>;

    void                    createBuffers(LogicalDevice& device);
    NO_DISCARD StagingBuffs createStaging(LogicalDevice& device);

    NO_DISCARD StagingBuffs createAllBuffs(LogicalDevice& device) {
      createBuffers(device);
      return createStaging(device);
    };

    void uploadStaging(StagingBuffs& buffs);
    void uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& staging) const;

    NO_DISCARD StagingBuffs upload(CommandBuffer& cmdBuff);
    void                    upload(CommandBuffer& cmdBuff, Queue& queue);

    // clears out the cached vertices/indices
    void clearCache();

    bool isDrawable() const;

    bool operator==(Mesh const& o) const;

  private:
    std::vector<Vertex>   m_vertices;
    std::vector<uint32_t> m_indices;
    util::ptr<Buffer> m_vertexBuff;
    util::ptr<Buffer> m_indexBuff;
  };
}

#endif
