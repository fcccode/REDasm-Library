#ifndef FORMAT_H
#define FORMAT_H

#include "../disassembler/disassemblerapi.h"
#include "../disassembler/types/symboltable.h"
#include "../disassembler/listing/listingdocument.h"
#include "../analyzer/analyzer.h"
#include "../types/endianness/endianness.h"
#include "base.h"

#define DECLARE_FORMAT_PLUGIN(T, id) inline FormatPlugin* id##_formatPlugin(AbstractBuffer* buffer) { return REDasm::getFormatPlugin<T>(buffer); }
#define DEFINE_FORMAT_PLUGIN_TEST(T) public: static bool test(const T* format, BufferView& buffer); private:
#define FORMAT_PLUGIN_TEST(T, H) bool T::test(const H* format, BufferView& view)

namespace REDasm {

template<typename T> T* getFormatPlugin(AbstractBuffer* buffer)
{
    const typename T::FormatHeader* format = reinterpret_cast<const typename T::FormatHeader*>(buffer->data());
    BufferView view = buffer->view();

    if((sizeof(typename T::FormatHeader) > buffer->size()) || !T::test(format, view))
        return nullptr;

    T* formatplugin = new T(buffer);
    formatplugin->load();
    return formatplugin;
}

class FormatPlugin: public Plugin
{
    public:
        FormatPlugin(AbstractBuffer *view);
        AbstractBuffer* buffer() const;
        BufferView viewOffset(offset_t offset) const;
        BufferView view(address_t address) const;
        BufferView viewSegment(const Segment* segment) const;
        ListingDocument& createDocument();
        ListingDocument& document();
        const SignatureFiles& signatures() const;
        u64 addressWidth() const;

    public:
        virtual bool isBinary() const;
        virtual offset_location offset(address_t address) const;
        virtual address_location address(offset_t offset) const;
        virtual Analyzer *createAnalyzer(DisassemblerAPI* disassembler, const SignatureFiles &signatures) const;
        virtual std::string assembler() const = 0;
        virtual u32 bits() const = 0;
        virtual void load() = 0;

    public:
        template<typename U> inline offset_location fileoffset(const U* ptr) const { return REDasm::make_location<offset_t>(reinterpret_cast<const u8*>(ptr) - reinterpret_cast<const u8*>(m_buffer->data()), m_view.inRange(ptr)); }
        template<typename U> inline address_location addressof(const U* ptr) const { return m_view.inRange(ptr) ? this->address(this->fileoffset(ptr)) : REDasm::invalid_location<address_t>();  }
        template<typename U, typename O> inline U* pointer(O offset) const { return m_view.inRange(offset) ? reinterpret_cast<U*>(reinterpret_cast<u8*>(m_buffer->data()) + offset) : nullptr; }
        template<typename U, typename A> inline U* addrpointer(A address) const { auto o = offset(address); return o ? reinterpret_cast<U*>(reinterpret_cast<u8*>(m_buffer->data()) + o) : nullptr; }
        template<typename U, typename V, typename O> inline static const U* relpointer(const V* base, O offset) { return reinterpret_cast<const U*>(reinterpret_cast<const u8*>(base) + offset); }
        template<typename U, typename V, typename O> inline static U* relpointer(V* base, O offset) { return reinterpret_cast<U*>(reinterpret_cast<u8*>(base) + offset); }

    protected:
        std::unique_ptr<AbstractBuffer> m_buffer;
        BufferView m_view;
        ListingDocument m_document;
        SignatureFiles m_signatures;
};

template<typename T> class FormatPluginT: public FormatPlugin
{
    public:
        typedef T FormatHeader;

    public:
        FormatPluginT(AbstractBuffer* buffer): FormatPlugin(buffer) { m_format = reinterpret_cast<T*>(m_buffer->data()); }
        static bool test(const T* format, const AbstractBuffer* buffer) { RE_UNUSED(format); RE_UNUSED(buffer); return false; }

    protected:
        T* m_format;
};

class FormatPluginB: public FormatPluginT<u8>
{
    public:
        FormatPluginB(AbstractBuffer* buffer): FormatPluginT<u8>(buffer) { }
};

typedef std::function<FormatPlugin*(AbstractBuffer*)> FormatPlugin_Entry;

}

#endif // FORMAT_H
