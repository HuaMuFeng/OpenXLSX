/*

   ____                               ____      ___ ____       ____  ____      ___
  6MMMMb                              `MM(      )M' `MM'      6MMMMb\`MM(      )M'
 8P    Y8                              `MM.     d'   MM      6M'    ` `MM.     d'
6M      Mb __ ____     ____  ___  __    `MM.   d'    MM      MM        `MM.   d'
MM      MM `M6MMMMb   6MMMMb `MM 6MMb    `MM. d'     MM      YM.        `MM. d'
MM      MM  MM'  `Mb 6M'  `Mb MMM9 `Mb    `MMd       MM       YMMMMb     `MMd
MM      MM  MM    MM MM    MM MM'   MM     dMM.      MM           `Mb     dMM.
MM      MM  MM    MM MMMMMMMM MM    MM    d'`MM.     MM            MM    d'`MM.
YM      M9  MM    MM MM       MM    MM   d'  `MM.    MM            MM   d'  `MM.
 8b    d8   MM.  ,M9 YM    d9 MM    MM  d'    `MM.   MM    / L    ,M9  d'    `MM.
  YMMMM9    MMYMMM9   YMMMM9 _MM_  _MM_M(_    _)MM_ _MMMMMMM MYMMMM9 _M(_    _)MM_
            MM
            MM
           _MM_

  Copyright (c) 2018, Kenneth Troldal Balslev

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  - Neither the name of the author nor the
    names of any contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

// ===== External Includes ===== //
// #include <algorithm>
#include <pugixml.hpp>

// ===== OpenXLSX Includes ===== //
#include "XLDocument.hpp"               // pugi_parse_settings
#include "XLComments.hpp"
#include "utilities/XLUtilities.hpp"    // OpenXLSX::ignore, appendAndGetNode

using namespace OpenXLSX;

namespace OpenXLSX
{
    // placeholder for utility functions

}    // namespace OpenXLSX

// ========== XLComments Member Functions

/**
 * @details The constructor creates an instance of the superclass, XLXmlFile
 */
XLComments::XLComments(XLXmlData* xmlData) : XLXmlFile(xmlData)
{
    if (xmlData->getXmlType() != XLContentType::Comments)
        throw XLInternalError("XLComments constructor: Invalid XML data.");

    XMLDocument & doc = xmlDocument();
    if (doc.document_element().empty())    // handle a bad (no document element) comments XML file
        doc.load_string(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<comments xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
                " xmlns:xdr=\"http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing\">\n"
                "</comments>",
                pugi_parse_settings
        );

    XMLNode rootNode = doc.document_element();
    bool docNew = rootNode.first_child_of_type(pugi::node_element).empty();   // check and store status: was document empty?
    m_authors = appendAndGetNode (rootNode, "authors", m_nodeOrder);          // (insert and) get authors node
    if (docNew) rootNode.prepend_child(pugi::node_pcdata).set_value("\n\t");  // if document was empty: prefix the newly inserted authors node with a single tab
    m_commentList = appendAndGetNode (rootNode, "commentList", m_nodeOrder);  // (insert and) get commentList node -> this should now copy the whitespace prefix of m_authors

    // whitespace formatting / indentation for closing tags:
    if (    m_authors.first_child().empty())     m_authors.append_child(pugi::node_pcdata).set_value("\n\t");
    if (m_commentList.first_child().empty()) m_commentList.append_child(pugi::node_pcdata).set_value("\n\t");
}


/**
 * @details TODO: write doxygen headers for functions in this module
 */
XMLNode XLComments::authorNode(uint16_t index) const
{
    XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
    uint16_t i = 0;
    while (not auth.empty() && i != index) {
        ++i;
        auth = auth.next_sibling_of_type(pugi::node_element);
    }
    return auth; // auth.empty() will be true if not found
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
XMLNode XLComments::commentNode(const std::string& cellRef) const
{
    return m_commentList.find_child_by_attribute("comment", "ref", cellRef.c_str());
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
uint16_t XLComments::authorCount() const
{
    XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
    uint16_t count = 0;
    while (not auth.empty()) {
        ++count;
        auth = auth.next_sibling_of_type(pugi::node_element);
    }
    return count;
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
std::string XLComments::author(uint16_t index) const
{
    XMLNode auth = authorNode(index);
    if (auth.empty()) {
        using namespace std::literals::string_literals;
        throw XLException("XLComments::author: index "s + std::to_string(index) + " is out of bounds"s);
    }
    return auth.first_child().value(); // author name is stored as a node_pcdata within the author node
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
bool XLComments::deleteAuthor(uint16_t index)
{
    XMLNode auth = authorNode(index);
    if (auth.empty()) {
        using namespace std::literals::string_literals;
        throw XLException("XLComments::deleteAuthor: index "s + std::to_string(index) + " is out of bounds"s);
    }
    else {
        while (auth.previous_sibling().type() == pugi::node_pcdata) // remove leading whitespaces
            m_authors.remove_child(auth.previous_sibling());
        m_authors.remove_child(auth);                             // then remove author node itself
    }
    return true;
}

/**
 * @details insert author and return index
 */
uint16_t XLComments::addAuthor(const std::string& authorName)
{
    XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
    uint16_t index = 0;
    while (not auth.next_sibling_of_type(pugi::node_element).empty()) {
        ++index;
        auth = auth.next_sibling_of_type(pugi::node_element);
    }
    if (auth.empty()) { // if this is the first entry
        auth = m_authors.prepend_child("author"); // insert new node
        m_authors.prepend_child(pugi::node_pcdata).set_value("\n\t\t");  // prefix first author with second level indentation
    }
    else { // found the last author node at index
        auth = m_authors.insert_child_after("author", auth);               // append a new author
        copyLeadingWhitespaces(m_authors, auth.previous_sibling(), auth ); // copy whitespaces prefix from previous author
        ++index;                                                           // increment index
    }
    auth.prepend_child(pugi::node_pcdata).set_value(authorName.c_str());
    return index;
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
size_t XLComments::count() const
{
    XMLNode comment = m_commentList.first_child_of_type(pugi::node_element);
    size_t count = 0;
    while (not comment.empty()) {
        // if (comment.name() == "comment") // TBD: safe-guard against potential rogue node
        ++count;
        comment = comment.next_sibling_of_type(pugi::node_element);
    }
    return count;
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
uint16_t XLComments::authorId(const std::string& cellRef) const
{
    XMLNode comment = commentNode(cellRef);
    return static_cast<uint16_t>(comment.attribute("authorId").as_uint());
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
bool XLComments::deleteComment(const std::string& cellRef)
{
    XMLNode comment = commentNode(cellRef);
    if (comment.empty()) return false;
    else
        m_commentList.remove_child(comment);
    return true;
}

// comment entries:
//  attribute ref -> cell
//  attribute authorId -> author index in array
//  node text
//     subnode t -> regular text
//        attribute xml:space="preserve" - seems useful to always apply
//        subnode pc_data -> the comment text
//     subnode r -> rich text, repetition of:
//        subnode rPr -> rich text formatting
//           subnode sz -> font size (int)
//           subnode rFont -> font name (string)
//           subnode family -> TBC: font family (int)
//        subnode t -> regular text like above

/**
 * @details TODO: write doxygen headers for functions in this module
 */
std::string XLComments::get(const std::string& cellRef) const
{
    std::string result{};
    XMLNode comment = commentNode(cellRef);

    using namespace std::literals::string_literals;
    XMLNode textElement = comment.child("text").first_child_of_type(pugi::node_element);
    while (not textElement.empty()) {
        if (textElement.name() == "t"s) {
            result += textElement.first_child().value();
        }
        else if (textElement.name() == "r"s) { // rich text
            XMLNode richTextSubnode = textElement.first_child_of_type(pugi::node_element);
            while (not richTextSubnode.empty()) {
                if (textElement.name() == "t"s) {
                    result += textElement.first_child().value();
                }
                else if (textElement.name() == "rPr"s) {} // ignore rich text formatting info
                else {} // ignore other nodes
                richTextSubnode = richTextSubnode.next_sibling_of_type(pugi::node_element);
            }
        }
        else {} // ignore other elements (for now)
        textElement = textElement.next_sibling_of_type(pugi::node_element);
    }
    return result;
}

/**
 * @details TODO: write doxygen headers for functions in this module
 */
bool XLComments::set(std::string cellRef, std::string commentText, uint16_t authorId)
{
    XLCellReference destRef(cellRef);
    uint32_t destRow = destRef.row();
    uint16_t destCol = destRef.column();

    using namespace std::literals::string_literals;
    XMLNode comment = m_commentList.first_child_of_type(pugi::node_element);
    while (not comment.empty()) {
        if (comment.name() == "comment"s) { // safeguard against rogue nodes
            XLCellReference ref(comment.attribute("ref").value());
            if (ref.row() > destRow ||(ref.row() == destRow && ref.column() >= destCol)) // abort when node or a node behind it is found
    break;
        }
        comment = comment.next_sibling_of_type(pugi::node_element);
    }
    if(comment.empty()) {                                                     // no comments yet or this will be the last comment
        comment = m_commentList.last_child_of_type(pugi::node_element);
        if (comment.empty()) {                                                   // if this is the only comment so far
            comment = m_commentList.prepend_child("comment");                                   // prepend new comment
            m_commentList.insert_child_before(pugi::node_pcdata, comment).set_value("\n\t\t");  // insert double indent before comment
        }
        else {
            comment = m_commentList.insert_child_after("comment", comment);                     // insert new comment at end of list
            copyLeadingWhitespaces(m_commentList, comment.previous_sibling(), comment);         // and copy whitespaces prefix from previous comment
        }
    }
    else {
        XLCellReference ref(comment.attribute("ref").value());
        if(ref.row() != destRow || ref.column() != destCol) {                 // if node has to be inserted *before* this one
            comment = m_commentList.insert_child_before("comment", comment);        // insert new comment
            copyLeadingWhitespaces(m_commentList, comment, comment.next_sibling()); // and copy whitespaces prefix from next node
        }
        else // node exists / was found
            comment.remove_children();    // clear node content
    }

    // now that we have a valid comment node: update attributes and content
    if (comment.attribute("ref").empty())                                  // if ref has to be created
        comment.append_attribute("ref").set_value(destRef.address().c_str()); // then do so - otherwise it can remain untouched
    appendAndSetAttribute(comment, "authorId", std::to_string(authorId));  // update authorId
    XMLNode tNode = comment.prepend_child("text").prepend_child("t");      // insert <text><t/></text> nodes
    tNode.append_attribute("xml:space").set_value("preserve");             // set <t> node attribute xml:space
    tNode.prepend_child(pugi::node_pcdata).set_value(commentText.c_str()); // finally, insert <t> node_pcdata value

    return true;
}

/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLComments::print(std::basic_ostream<char>& ostr) const { xmlDocument().document_element().print( ostr ); }
