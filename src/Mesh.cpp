// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * gproj : Mesh.cpp
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

#include "Mesh.h"

#include "CommandBuffer.h"
#include "Queue.h"

namespace dw {
  Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)) {
  }

  Mesh::Mesh(Mesh&& o) noexcept
    : m_vertices(std::move(o.m_vertices)),
      m_indices(std::move(o.m_indices)),
      m_vertexBuff(std::move(o.m_vertexBuff)),
      m_indexBuff(std::move(o.m_indexBuff)),
      m_material(std::move(o.m_material)) {
    o.m_vertexBuff = nullptr;
    o.m_indexBuff = nullptr;
    o.m_material = nullptr;
  }

  Mesh::~Mesh() {
    if (m_vertexBuff)
      m_vertexBuff.reset();

    if (m_indexBuff)
      m_indexBuff.reset();

    if (m_material)
      m_material.reset();
  }

  void Mesh::setMaterial(util::ptr<Material> mtl) {
    m_material = std::move(mtl);
  }

  util::ptr<Material> Mesh::getMaterial() const {
    return m_material;
  }

  Buffer& Mesh::getIndexBuffer() {
    return *m_indexBuff;
  }

  Buffer& Mesh::getVertexBuffer() {
    return *m_vertexBuff;
  }

  Mesh& Mesh::operator=(Mesh&& o) noexcept {
    m_vertices = std::move(o.m_vertices);
    m_indices = std::move(o.m_indices);
    m_vertexBuff = std::move(o.m_vertexBuff);
    m_indexBuff = std::move(o.m_indexBuff);
    m_material = std::move(m_material);
    return *this;
  }

  size_t Mesh::getSizeOfIndices() const {
    return m_indices.size() * sizeof(uint32_t);
  }

  size_t Mesh::getSizeOfVertices() const {
    return m_vertices.size() * sizeof(Vertex);
  }

  size_t Mesh::getIndexSize() const {
    return sizeof(uint32_t);
  }

  size_t Mesh::getVertexSize() const {
    return sizeof(Vertex);
  }

  size_t Mesh::getNumIndices() const {
    return m_indices.size();
  }

  size_t Mesh::getNumVertices() const {
    return m_vertices.size();
  }

  void Mesh::createBuffers(LogicalDevice& device) {
    auto vertSize = getSizeOfVertices();
    auto indexSize = getSizeOfIndices();

    m_vertexBuff = std::make_unique<Buffer>(Buffer::CreateVertex(device, vertSize));
    m_indexBuff = std::make_unique<Buffer>(Buffer::CreateIndex(device, indexSize));
  }

  void Mesh::uploadStaging(StagingBuffs& buffs) {
    void* data = buffs.first.map();
    memcpy(data, m_vertices.data(), getSizeOfVertices());
    buffs.first.unMap();

    data = buffs.second.map();
    memcpy(data, m_indices.data(), getSizeOfIndices());
    buffs.second.unMap();
  }

  Mesh::StagingBuffs Mesh::createStaging(LogicalDevice& device) {
    return std::make_pair(
      Buffer::CreateStaging(device, getSizeOfVertices()),
      Buffer::CreateStaging(device, getSizeOfIndices())
    );
  }

  void Mesh::uploadCmds(CommandBuffer& cmdBuff, StagingBuffs& buffs) const {
    VkBufferCopy vertCopy = { 0, 0, getSizeOfVertices() };
    VkBufferCopy indexCopy = { 0, 0, getSizeOfIndices() };

    assert(m_vertexBuff);
    assert(m_indexBuff);

    vkCmdCopyBuffer(cmdBuff, buffs.first, *m_vertexBuff, 1, &vertCopy);
    vkCmdCopyBuffer(cmdBuff, buffs.second, *m_indexBuff, 1, &indexCopy);
  }

  Mesh::StagingBuffs Mesh::upload(CommandBuffer& cmdBuff) {
    LogicalDevice& device = cmdBuff.getOwningPool().getOwningDevice();
    auto vertSize = getSizeOfVertices();
    auto indexSize = getSizeOfIndices();

    createBuffers(device);
    auto staging = createStaging(device);
    uploadStaging(staging);
    uploadCmds(cmdBuff, staging);

    return staging;
  }

  void Mesh::upload(CommandBuffer& buffer, Queue& queue) {
    buffer.start(true);
    auto staging = upload(buffer);
    buffer.end();

    queue.submitOne(buffer);
    queue.waitSubmit();
  }

  void Mesh::clearCache() {
    m_vertices.clear();
    m_indices.clear();
  }


  bool Mesh::isDrawable() const {
    return m_vertexBuff != nullptr;
  }

  bool Mesh::operator==(Mesh const& o) const {
    return m_vertexBuff == o.m_vertexBuff;
  }
}
