/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:  
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xalan" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written 
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */



// Class header file.
#include "XSLTEngineImpl.hpp"


#include <memory>



#include <XalanDOM/XalanDOMException.hpp>
#include <XalanDOM/XalanNode.hpp>
#include <XalanDOM/XalanAttr.hpp>
#include <XalanDOM/XalanComment.hpp>
#include <XalanDOM/XalanCDATASection.hpp>
#include <XalanDOM/XalanNodeList.hpp>
#include <XalanDOM/XalanNamedNodeMap.hpp>
#include <XalanDOM/XalanProcessingInstruction.hpp>
#include <XalanDOM/XalanText.hpp>



#include <sax/DocumentHandler.hpp>
#include <sax/Locator.hpp>
#include <sax/SAXException.hpp>
#include <util/PlatformUtils.hpp>
#include <framework/URLInputSource.hpp>



#include <PlatformSupport/DOMStringPrintWriter.hpp>
#include <PlatformSupport/PrintWriter.hpp>
#include <PlatformSupport/STLHelper.hpp>
#include <PlatformSupport/StringTokenizer.hpp>
#include <PlatformSupport/XalanUnicode.hpp>



#include <DOMSupport/DOMServices.hpp>



#include <XMLSupport/Formatter.hpp>
#include <XMLSupport/FormatterToDOM.hpp>
#include <XMLSupport/FormatterToText.hpp>
#include <XMLSupport/FormatterToXML.hpp>
#include <XMLSupport/FormatterToHTML.hpp>
#include <XMLSupport/FormatterTreeWalker.hpp>
#include <XMLSupport/XMLParserLiaison.hpp>
#include <XMLSupport/FormatterTreeWalker.hpp>



#include <XPath/ElementPrefixResolverProxy.hpp>
#include <XPath/ResultTreeFrag.hpp>
#include <XPath/XObject.hpp>
#include <XPath/XPathEnvSupport.hpp>
#include <XPath/XPathExecutionContextDefault.hpp>
#include <XPath/XPathFactory.hpp>
#include <XPath/XPathProcessorImpl.hpp>
#include <XPath/XPathSupport.hpp>
#include <XPath/XObject.hpp>
#include <XPath/XObjectFactory.hpp>
#include <XPath/XResultTreeFrag.hpp>



#include "Constants.hpp"
#include "ElemWithParam.hpp"
#include "FunctionCurrent.hpp"
#include "FunctionDocument.hpp"
#include "FunctionElementAvailable.hpp"
#include "FunctionFunctionAvailable.hpp"
#include "FunctionFormatNumber.hpp"
#include "FunctionGenerateID.hpp"
#include "FunctionKey.hpp"
#include "FunctionSystemProperty.hpp"
#include "FunctionUnparsedEntityURI.hpp"
#include "GenerateEvent.hpp"
#include "ProblemListener.hpp"
#include "ProblemListenerDefault.hpp"
#include "Stylesheet.hpp"
#include "StylesheetConstructionContext.hpp"
#include "StylesheetExecutionContext.hpp"
#include "StylesheetHandler.hpp"
#include "StylesheetRoot.hpp"
#include "TraceListener.hpp"
#include "XSLTInputSource.hpp"
#include "XSLTProcessorException.hpp"



//==========================================================
// SECTION: Constructors
//==========================================================

XSLTEngineImpl::XSLTEngineImpl(
			XMLParserLiaison&	parserLiaison,
			XPathSupport&		xpathSupport,
			XPathEnvSupport&	xpathEnvSupport,
			XObjectFactory&		xobjectFactory,
			XPathFactory&		xpathFactory) :
	XSLTProcessor(),
	DocumentHandler(),
	m_outputCarriageReturns(false),
	m_outputLinefeeds(false),
	m_resultTreeFactory(0),
	m_resultNameSpacePrefix(),
	m_resultNameSpaceURL(),
	m_currentNode(),
	m_pendingElementName(),
	m_pendingAttributes(),
	m_hasPendingStartDocument(false),
	m_mustFlushStartDocument(false),
	m_resultNameSpaces(),
	m_emptyNamespace(),
	m_xpathFactory(xpathFactory),
	m_xobjectFactory(xobjectFactory),
	m_xpathProcessor(new XPathProcessorImpl),
	m_cdataStack(),
	m_stylesheetLocatorStack(),
	m_defaultProblemListener(),
	m_problemListener(&m_defaultProblemListener),
	m_stylesheetRoot(0),
	m_traceSelects(false),
	m_quietConflictWarnings(false),
	m_diagnosticsPrintWriter(0),
	m_durationsTable(),
	m_traceListeners(),
	m_uniqueNSValue(0),
	m_stripWhiteSpace(false),
	m_topLevelParams(),
	m_parserLiaison(parserLiaison),
	m_xpathSupport(xpathSupport),
	m_xpathEnvSupport(xpathEnvSupport),
	m_flistener(0),
	m_executionContext(0)
{
}



void
XSLTEngineImpl::reset()
{
	m_topLevelParams.clear();
	m_durationsTable.clear();
	m_stylesheetLocatorStack.clear();
	clear(m_pendingElementName);
	m_pendingAttributes.clear();
	m_cdataStack.clear();
	m_resultTreeFactory = 0;
	m_currentNode = 0;

	m_hasPendingStartDocument = false;
	m_mustFlushStartDocument = false;

	m_xpathSupport.reset();
	m_xpathEnvSupport.reset();
	m_xpathFactory.reset();
	m_xobjectFactory.reset();
}



XSLTEngineImpl::~XSLTEngineImpl()
{
	reset();
}



//==========================================================
// SECTION: Main API Functions
//==========================================================



AttributeListImpl& 
XSLTEngineImpl::getPendingAttributes()
{
	return m_pendingAttributes;
}



const XalanDOMString
XSLTEngineImpl::getPendingElementName() const
{
	return m_pendingElementName;
}



void
XSLTEngineImpl::setPendingAttributes(const AttributeList&	pendingAttributes)
{
	m_pendingAttributes = pendingAttributes;
}	



void
XSLTEngineImpl::setPendingElementName(const XalanDOMString&	elementName)
{
	m_pendingElementName = elementName;
}



void
XSLTEngineImpl::process(
			XSLTInputSource&				inputSource, 
	        XSLTInputSource&				stylesheetSource,
	        XSLTResultTarget&				outputTarget,
			StylesheetConstructionContext&	constructionContext,
			StylesheetExecutionContext&		executionContext)
{
	try
	{
		XalanDOMString	xslIdentifier;

		if (0 == stylesheetSource.getSystemId())
		{
			xslIdentifier = XalanDOMString(XALAN_STATIC_UCODE_STRING("Input XSL"));
		}
		else
		{
			xslIdentifier = stylesheetSource.getSystemId();
		}

		bool totalTimeID = true;

		pushTime(&totalTimeID);

		XalanNode*	sourceTree = getSourceTreeFromInput(inputSource);

		try
		{
			m_stylesheetRoot = processStylesheet(stylesheetSource, constructionContext);
		}
		catch(const XSLException&)
		{
		}
		catch(const SAXException&)
		{
		}
		catch(const XMLException&)
		{
		}

		if(0 != sourceTree && m_stylesheetRoot == 0)
		{
			// Didn't get a stylesheet from the input source, so look for a
			// stylesheet processing instruction...
			XalanDOMString			stylesheetURI = 0;

			// The PI must be a child of the document...
			XalanNode*				child = sourceTree->getFirstChild();

#if !defined(XALAN_NO_NAMESPACES)
			using std::vector;
#endif

			vector<XalanDOMString>	hrefs;

			// $$$ ToDo: is this first one style valid?
			const XalanDOMString	stylesheetNodeName1(XALAN_STATIC_UCODE_STRING("xml-stylesheet"));
			const XalanDOMString	stylesheetNodeName2(XALAN_STATIC_UCODE_STRING("xml:stylesheet"));

			// $$$ ToDo: This code is much like that in getStyleSheetURIFromDoc().
			// Why is it repeated???
			// $$$ ToDo: Move these embedded strings from inside these loops
			// out here...
			// $$$ ToDo: These loops are a mess of repeated use of the
			// same data values.
			while(child != 0)
			{
				if(XalanNode::PROCESSING_INSTRUCTION_NODE == child->getNodeType())
				{
					const XalanDOMString	nodeName(child->getNodeName());

					if(equals(nodeName, stylesheetNodeName1) ||
					   equals(nodeName, stylesheetNodeName2))
					{
						bool isOK = true;

						StringTokenizer 	tokenizer(child->getNodeValue(), XALAN_STATIC_UCODE_STRING(" \t="));

						while(tokenizer.hasMoreTokens())
						{
							if(equals(tokenizer.nextToken(), XALAN_STATIC_UCODE_STRING("type")))
							{
								XalanDOMString	typeVal = tokenizer.nextToken();

								typeVal = substring(typeVal, 1, length(typeVal) - 1);

								if(!equals(typeVal, XALAN_STATIC_UCODE_STRING("text/xsl")))
								{
									isOK = false;
								}
							}
						}	
						
						if(isOK)
						{
							StringTokenizer 	tokenizer(child->getNodeValue(), XALAN_STATIC_UCODE_STRING(" \t="));

							while(tokenizer.hasMoreTokens())
							{
								const XalanDOMString	theCurrentToken = tokenizer.nextToken();

								if(equals(theCurrentToken, XALAN_STATIC_UCODE_STRING("href")))
								{
									stylesheetURI = tokenizer.nextToken();
									stylesheetURI = substring(stylesheetURI, 1, length(stylesheetURI) - 1);
									hrefs.push_back(stylesheetURI);
								}
							}
						}
					}
				}

				child = child->getNextSibling();
			}

			bool isRoot = true;
			Stylesheet* prevStylesheet = 0;
			while(!hrefs.empty())
			{
				const XalanDOMChar* const		pxch = inputSource.getSystemId();
				const XalanDOMString	sysid(pxch);
				const XalanDOMString&	ref =  hrefs.back();

				Stylesheet* stylesheet =
					getStylesheetFromPIURL(ref, *sourceTree, sysid, isRoot, constructionContext);

				if(false == isRoot)
				{
					prevStylesheet->addImport(stylesheet, false);
				}

				prevStylesheet = stylesheet;
				isRoot = false;
				hrefs.pop_back();
			}
		}

		if(0 == m_stylesheetRoot)
		{
			error("Failed to process stylesheet!");
		}
		else if(0 != sourceTree)
		{
			executionContext.setStylesheetRoot(m_stylesheetRoot);

			m_stylesheetRoot->process(sourceTree, outputTarget, executionContext);

			if(0 != m_diagnosticsPrintWriter)
			{
				displayDuration(XALAN_STATIC_UCODE_STRING("Total time"), &totalTimeID);
			}
		}
	}
	catch(SAXException& se)
	{
		message("SAX Exception");

		throw se;
	}
	catch (...)
	{
		message("Unknown Exception");

		throw;
	}
}



void
XSLTEngineImpl::process(
			XSLTInputSource&				inputSource, 
	        XSLTResultTarget&				outputTarget,
			StylesheetExecutionContext&		executionContext)
{
	bool	totalTimeID = true;

	if(0 != m_diagnosticsPrintWriter)
	{
		pushTime(&totalTimeID);
	}

	XalanNode* const	sourceTree = getSourceTreeFromInput(inputSource);

	if(0 != sourceTree)
	{
		if (m_stylesheetRoot == 0)
		{
			error("No stylesheet is available to process!");
		}

		m_stylesheetRoot->process(sourceTree, outputTarget, executionContext);
	}

	if(0 != m_diagnosticsPrintWriter)
	{
		displayDuration(XALAN_STATIC_UCODE_STRING("Total time"), &totalTimeID);
	}
}



StylesheetRoot*
XSLTEngineImpl::processStylesheet(
			const XalanDOMString&			xsldocURLString,
			StylesheetConstructionContext&	constructionContext)
{
	try
	{
		XSLTInputSource		input(c_wstr(xsldocURLString));

		return processStylesheet(input, constructionContext);
	}
	catch(SAXException& se)
	{
		message("processStylesheet not successfull!");

		throw se;
	}

	return 0;
}



StylesheetRoot*
XSLTEngineImpl::processStylesheet(
  			XSLTInputSource&				stylesheetSource,
			StylesheetConstructionContext&	constructionContext)
{
	StylesheetRoot*		theStylesheet = 0;

	const XalanDOMChar* const	systemID = stylesheetSource.getSystemId();
	XalanNode* const			stylesheetNode = stylesheetSource.getNode();

	if (systemID != 0 || stylesheetNode != 0 || stylesheetSource.getStream() != 0)
	{
		XalanDOMString	xslIdentifier;

		try
		{
			theStylesheet = constructionContext.create(stylesheetSource);

			StylesheetHandler	stylesheetProcessor(*theStylesheet, constructionContext);

			if(stylesheetNode != 0)
			{
				xslIdentifier = XALAN_STATIC_UCODE_STRING("Input XSL");

				FormatterTreeWalker tw(stylesheetProcessor);

				tw.traverse(stylesheetSource.getNode());
			}
			else
			{
				xslIdentifier = systemID;

				diag(XALAN_STATIC_UCODE_STRING("========= Parsing ") + xslIdentifier + XALAN_STATIC_UCODE_STRING(" =========="));
				pushTime(&xslIdentifier);
				m_parserLiaison.parseXMLStream(stylesheetSource,
											   stylesheetProcessor);
				if(0 != m_diagnosticsPrintWriter)
					displayDuration(XALAN_STATIC_UCODE_STRING("Parse of ") + xslIdentifier, &xslIdentifier);
			}

			theStylesheet->postConstruction();
		}
		catch(const XSLException&)
		{
			message("Error parsing " + xslIdentifier);

			throw;
		}
		catch(const SAXException&)
		{
			message("Error parsing " + xslIdentifier);

			throw;
		}
		catch(const XMLException&)
		{
			message("Error parsing " + xslIdentifier);

			throw;
		}
	}

	return theStylesheet;
}



//==========================================================
// SECTION: XML Parsing Functions
//==========================================================

XalanNode*
XSLTEngineImpl::getSourceTreeFromInput(XSLTInputSource&		inputSource)
{
	XalanNode*		sourceTree = 0;

	XalanDOMString	xmlIdentifier = 0 != inputSource.getSystemId() ?
											inputSource.getSystemId() :
											XALAN_STATIC_UCODE_STRING("Input XML");

	if(0 != inputSource.getNode())
	{
		sourceTree = inputSource.getNode();
	}
	else
	{
		// In case we have a fragment identifier, go ahead and 
		// try to parse the XML here.
		try
		{
			diag(XALAN_STATIC_UCODE_STRING("========= Parsing ") +
					xmlIdentifier +
					XALAN_STATIC_UCODE_STRING(" =========="));

			pushTime(&xmlIdentifier);

			XalanDocument* const	theDocument =
						m_parserLiaison.parseXMLStream(inputSource,
													   xmlIdentifier);
			assert(theDocument != 0);

			if(0 != m_diagnosticsPrintWriter)
				displayDuration(XALAN_STATIC_UCODE_STRING("Parse of ") +
									xmlIdentifier,
								&xmlIdentifier);

			m_xpathEnvSupport.setSourceDocument(xmlIdentifier, theDocument);

			sourceTree = theDocument;
		}
		// catch(Exception e)
		// $$$ ToDo: Fix this!!!
		catch(...)
		{
			error("Could not parse " + xmlIdentifier + " document!");
		}
	}

	return sourceTree;
}



XalanDocument*
XSLTEngineImpl::parseXML(
			const XalanDOMString&	urlString,
			DocumentHandler*		docHandler,
			XalanDocument*			docToRegister)
{
	
	XalanDocument*	doc =
			m_xpathEnvSupport.getSourceDocument(urlString);

	if(doc == 0)
	{
		XSLTInputSource		inputSource(c_wstr(urlString));

		if(0 != docHandler)
		{
			m_parserLiaison.parseXMLStream(inputSource, *docHandler);

			doc = docToRegister;
		}
		else
		{
			doc = m_parserLiaison.parseXMLStream(inputSource);
		}

		if (doc != 0)
		{
			m_xpathEnvSupport.setSourceDocument(urlString, doc);
		}
	}

	return doc;
}



Stylesheet*
XSLTEngineImpl::getStylesheetFromPIURL(
			const XalanDOMString&			xslURLString,
			XalanNode&						fragBase,
			const XalanDOMString&			xmlBaseIdent,
			bool							isRoot,
			StylesheetConstructionContext&	constructionContext)
{
	Stylesheet*				stylesheet = 0;

	XalanDOMString			stringHolder;

	const XalanDOMString	localXSLURLString = clone(trim(xslURLString));

	const unsigned int		fragIndex = indexOf(localXSLURLString, XalanUnicode::charNumberSign);

	const XalanDocument*	stylesheetDoc = 0;

	if(fragIndex == 0)
	{
		diag("Locating stylesheet from fragment identifier...");

		const XalanDOMString	fragID = substring(localXSLURLString, 1);

		const XalanElement*		nsNode = 0;

		if (fragBase.getNodeType() == XalanNode::DOCUMENT_NODE)
		{
			const XalanDocument&	doc =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanDocument&)fragBase;
#else
				static_cast<const XalanDocument&>(fragBase);
#endif

			nsNode = doc.getDocumentElement(); 
		}
		else if	(fragBase.getNodeType() == XalanNode::ELEMENT_NODE)
		{
#if defined(XALAN_OLD_STYLE_CASTS)
			nsNode = (const XalanElement*)&fragBase;
#else
			nsNode = static_cast<const XalanElement*>(&fragBase);
#endif
		}
		else
		{
			XalanNode* const	node = fragBase.getParentNode();

			if	(node->getNodeType() == XalanNode::ELEMENT_NODE) 
			{
#if defined(XALAN_OLD_STYLE_CASTS)
				nsNode = (const XalanElement*)&fragBase;
#else
				nsNode = static_cast<XalanElement*>(node);
#endif
			}
			else
			{
				error("Could not identify fragment: " + fragID);
			}
		}

		// Try a bunch of really ugly stuff to find the fragment.
		// What's the right way to do this?
		XalanDOMString	ds(XALAN_STATIC_UCODE_STRING("id("));

		ds += fragID;
		ds += XALAN_STATIC_UCODE_STRING(")");

		assert(nsNode != 0);

		ElementPrefixResolverProxy		theProxy(nsNode, m_xpathEnvSupport, m_xpathSupport);

		XPathExecutionContextDefault	theExecutionContext(m_xpathEnvSupport,
															m_xpathSupport,
															m_xobjectFactory,
															&fragBase,
															0,
															&theProxy);

		const XObjectGuard		xobj(
						m_xobjectFactory,
						evalXPathStr(ds, theExecutionContext));
		assert(xobj.get() != 0);

		const NodeRefListBase* nl = &xobj->nodeset();

		if(nl->getLength() == 0)
		{
			NodeRefList		theEmptyList;

			ds = XALAN_STATIC_UCODE_STRING("//*[@id='");
			ds += fragID;
			ds += XALAN_STATIC_UCODE_STRING("']");

			theExecutionContext.setContextNodeList(theEmptyList);

			const XObjectGuard		xobj(
						m_xobjectFactory,
						evalXPathStr(ds, theExecutionContext));
			assert(xobj.get() != 0);

			nl = &xobj->nodeset();

			if(nl->getLength() == 0)
			{
				ds = XALAN_STATIC_UCODE_STRING("//*[@name='");
				ds += fragID;
				ds += XALAN_STATIC_UCODE_STRING("']");

				theExecutionContext.setContextNodeList(theEmptyList);

				const XObjectGuard		xobj(
							m_xobjectFactory,
							evalXPathStr(ds, theExecutionContext));
				assert(xobj.get() != 0);

				nl = &xobj->nodeset();

				if(nl->getLength() == 0)
				{
					// Well, hell, maybe it's an XPath...
					theExecutionContext.setContextNodeList(theEmptyList);

					const XObjectGuard		xobj(
								m_xobjectFactory,
								evalXPathStr(fragID, theExecutionContext));
					assert(xobj.get() != 0);

					nl = &xobj->nodeset();
				}
			}
		}

		if(nl->getLength() == 0)
		{
			error("Could not find fragment: " + fragID);
		}

		XalanNode* const	frag = nl->item(0);

		if(XalanNode::ELEMENT_NODE == frag->getNodeType())
		{
			pushTime(frag);

			if(isRoot)
			{
				StylesheetRoot* const	theLocalRoot =
					constructionContext.create(stringHolder);

				stylesheet = theLocalRoot;

				m_stylesheetRoot = theLocalRoot;
			}
			else
			{
#if defined(XALAN_OLD_STYLE_CASTS)
				stylesheet = constructionContext.create(*((StylesheetRoot*)m_stylesheetRoot), stringHolder);
#else
				stylesheet = constructionContext.create(*const_cast<StylesheetRoot*>(m_stylesheetRoot), stringHolder);
#endif
			}

			StylesheetHandler stylesheetProcessor(*stylesheet, constructionContext);

			FormatterTreeWalker tw(stylesheetProcessor);

			tw.traverse(frag);

			displayDuration(XalanDOMString(XALAN_STATIC_UCODE_STRING("Setup of ")) +
								localXSLURLString,
								&frag);

			stylesheet->postConstruction();
		}
		else
		{
			stylesheetDoc = 0;
			error("Node pointed to by fragment identifier was not an element: " + fragID);
		}
	}
	else
	{ 
		// hmmm.. for now I'll rely on the XML parser to handle 
		// fragment URLs.
		diag(XalanDOMString(XALAN_STATIC_UCODE_STRING("========= Parsing and preparing ")) +
				localXSLURLString +
				XALAN_STATIC_UCODE_STRING(" =========="));
		pushTime(&localXSLURLString);

		if(isRoot)
		{
			StylesheetRoot* const	theLocalRoot =
					constructionContext.create(localXSLURLString);

			stylesheet = theLocalRoot;

			m_stylesheetRoot = theLocalRoot;
		}
		else
		{
#if defined(XALAN_OLD_STYLE_CASTS)
			stylesheet = new Stylesheet(*(StylesheetRoot*)m_stylesheetRoot, localXSLURLString, constructionContext);
#else
			stylesheet = new Stylesheet(*const_cast<StylesheetRoot*>(m_stylesheetRoot), localXSLURLString, constructionContext);
#endif
		}

		StylesheetHandler stylesheetProcessor(*stylesheet, constructionContext);

		typedef StylesheetConstructionContext::URLAutoPtrType	URLAutoPtrType;

		URLAutoPtrType	xslURL(constructionContext.getURLFromString(localXSLURLString, xmlBaseIdent));

		XSLTInputSource		inputSource(xslURL->getURLText());

		m_parserLiaison.parseXMLStream(inputSource, stylesheetProcessor);

		stylesheet->postConstruction();

		displayDuration("Parsing and init of " + localXSLURLString, &localXSLURLString);
	}

	return stylesheet;
}


//==========================================================
// SECTION: Stylesheet Tables
//==========================================================


double
XSLTEngineImpl::getXSLTVerSupported()
{
	return s_XSLTVerSupported;
}


//==========================================================
// SECTION: XSL directive handling functions
//==========================================================  



int
XSLTEngineImpl::getXSLToken(const XalanNode&	node) const
{
	int 	tok = -2;

	if(XalanNode::ELEMENT_NODE != node.getNodeType()) return tok;

	const XalanDOMString 	ns =
			m_xpathSupport.getNamespaceOfNode(node);

	if(equals(ns, s_XSLNameSpaceURL))
	{
		const XalanDOMString 	localName =
			m_xpathSupport.getLocalNameOfNode(node);

		const ElementKeysMapType::const_iterator		j =
						s_elementKeys.find(localName);

		if(j != s_elementKeys.end())
		{
			tok = (*j).second;
		}
	}
	else if(equals(ns, s_XSLT4JNameSpaceURL))
	{
		const XalanDOMString	localName =
			m_xpathSupport.getLocalNameOfNode(node);

		const ElementKeysMapType::const_iterator		j =
						s_XSLT4JElementKeys.find(localName);

		if(j != s_XSLT4JElementKeys.end())
		{
			tok = (*j).second;
		}
	}

	return tok;
}



bool
XSLTEngineImpl::isXSLTagOfType(const XalanNode&	node,
							int 		tagType) const
{
	return getXSLToken(node) == tagType ? true : false;
}



void
XSLTEngineImpl::outputToResultTree(
			StylesheetExecutionContext&		executionContext,
			const XObject&					value)
{
	const XObject::eObjectType	type = value.getType();

	XalanDOMString s;

	switch(type)
	{
	case XObject::eTypeBoolean:
	case XObject::eTypeNumber:
	case XObject::eTypeString:
		s = value.str();
		characters(toCharArray(s), 0, length(s));
		break;				

	case XObject::eTypeNodeSet:
		{
			const NodeRefListBase&	nl = value.nodeset();
			const unsigned int		nChildren = nl.getLength();

			for(unsigned int i = 0; i < nChildren; i++)
			{
				XalanNode*			pos = nl.item(i);
				XalanNode* const	top = pos;

				while(0 != pos)
				{
					flushPending();

					cloneToResultTree(*pos, false, false, true);

					XalanNode*	nextNode = pos->getFirstChild();

					while(0 == nextNode)
					{
						if(XalanNode::ELEMENT_NODE == pos->getNodeType())
						{
							endElement(c_wstr(pos->getNodeName()));
						}

						if(top == pos)
							break;

						nextNode = pos->getNextSibling();

						if(0 == nextNode)
						{
							pos = pos->getParentNode();

							if(top == pos)
							{
								if(XalanNode::ELEMENT_NODE == pos->getNodeType())
								{
									endElement(c_wstr(pos->getNodeName()));
								}

								nextNode = 0;
								break;
							}
						}
					}

					pos = nextNode;
				}
			}
		}
		break;
		
	case XObject::eTypeResultTreeFrag:
		outputResultTreeFragment(executionContext, value);
		break;

	case XObject::eTypeNull:
	case XObject::eTypeUnknown:
	case XObject::eUnknown:
	default:
		assert(0);
	}
}



const StylesheetRoot*
XSLTEngineImpl::getStylesheetRoot() const
{
	return m_stylesheetRoot;
}



void
XSLTEngineImpl::setStylesheetRoot(const StylesheetRoot*		theStylesheet)
{
	m_stylesheetRoot = theStylesheet;
}



void
XSLTEngineImpl::setExecutionContext(StylesheetExecutionContext*		theExecutionContext)
{
	m_executionContext = theExecutionContext;
}



//==========================================================
// SECTION: Diagnostic functions
//==========================================================

unsigned long
XSLTEngineImpl::getTraceListeners() const
{
	return m_traceListeners.size();
}



void
XSLTEngineImpl::addTraceListener(TraceListener* tl)
{
	if (tl != 0)
	{
		m_traceListeners.push_back(tl);
	}
}



void
XSLTEngineImpl::removeTraceListener(TraceListener*	tl)
{
#if !defined(XALAN_NO_NAMESPACES)
	using std::remove;
#endif

	const TraceListenerVectorType::iterator		i =
		remove(
			m_traceListeners.begin(),
			m_traceListeners.end(),
			tl);

	m_traceListeners.erase(i);
}



void
XSLTEngineImpl::fireGenerateEvent(const GenerateEvent&	ge)
{
#if !defined(XALAN_NO_NAMESPACES)
	using std::for_each;
#endif

	for_each(
		m_traceListeners.begin(),
		m_traceListeners.end(),
		TraceListener::TraceListenerGenerateFunctor(ge));
}



void
XSLTEngineImpl::fireSelectEvent(const SelectionEvent&	se)
{
#if !defined(XALAN_NO_NAMESPACES)
	using std::for_each;
#endif

	for_each(
		m_traceListeners.begin(),
		m_traceListeners.end(),
		TraceListener::TraceListenerSelectFunctor(se));
}



void
XSLTEngineImpl::fireTraceEvent(const TracerEvent& te)
{
#if !defined(XALAN_NO_NAMESPACES)
	using std::for_each;
#endif

	for_each(
		m_traceListeners.begin(),
		m_traceListeners.end(),
		TraceListener::TraceListenerTraceFunctor(te));
}



bool
XSLTEngineImpl::getTraceSelects() const
{
	return m_traceSelects;
}



void
XSLTEngineImpl::setTraceSelects(bool	b)
{
	m_traceSelects = b;
}



void
XSLTEngineImpl::message(
			const XalanDOMString&	msg,
			const XalanNode*		styleNode,
			const XalanNode*		sourceNode) const
{
	if (m_problemListener != 0)
	{
		const bool	shouldThrow =
			m_problemListener->problem(ProblemListener::eXSLPROCESSOR, 
									   ProblemListener::eMESSAGE,
									   styleNode, sourceNode,
									   c_wstr(msg), 0, 0, 0);

		if(shouldThrow == true)
		{
			throw XSLTProcessorException(msg);
		}
	}
}



void
XSLTEngineImpl::problem(
			const XalanDOMString&				msg, 
			ProblemListener::eClassification	classification,
			const XalanNode*					styleNode,
			const XalanNode*					sourceNode) const
{
	if (m_problemListener == 0) return;

	const Locator* const	locator = getLocatorFromStack();

	const XalanDOMChar* id = (0 == locator) ?
						0 : (0 == locator->getPublicId()) ?
					 locator->getPublicId() : locator->getSystemId();

	const bool	shouldThrow =
		m_problemListener->problem(
				ProblemListener::eXSLPROCESSOR, 
				classification,
				styleNode,
				sourceNode,
				msg, 
				id, 
				(0 == locator) ? 0: locator->getLineNumber(), 
				(0 == locator) ? 0: locator->getColumnNumber());

	if(shouldThrow == true)
	{
		throw XSLTProcessorException(msg);
	}
}



void
XSLTEngineImpl::warn(
			const XalanDOMString&	msg,
			const XalanNode*		styleNode,
			const XalanNode*		sourceNode) const
{
	problem(msg, ProblemListener::eWARNING, styleNode, sourceNode);
}


void
XSLTEngineImpl::error(
			const XalanDOMString&	msg,
			const XalanNode*		styleNode,
			const XalanNode*		sourceNode) const
{
	problem(msg, ProblemListener::eERROR, styleNode, sourceNode);
}



void
XSLTEngineImpl::pushTime(const void*	key) const
{
	if(0 != key)
	{
#if defined(XALAN_NO_MUTABLE)
		((XSLTEngineImpl*)this)->m_durationsTable.insert(DurationsTableMapType::value_type(key, clock()));
#else
		m_durationsTable.insert(DurationsTableMapType::value_type(key, clock()));
#endif
	}
}



clock_t
XSLTEngineImpl::popDuration(const void*		key) const
{
	clock_t 	clockTicksDuration = 0;

	if(0 != key)
	{
		const DurationsTableMapType::iterator	i =
#if defined(XALAN_NO_MUTABLE)
				((XSLTEngineImpl*)this)->m_durationsTable.find(key);
#else
				m_durationsTable.find(key);
#endif

		assert(i != m_durationsTable.end());

		if (i != m_durationsTable.end())
		{
			clockTicksDuration = clock() - (*i).second;

#if defined(XALAN_NO_MUTABLE)
			((XSLTEngineImpl*)this)->m_durationsTable.erase(i);
#else
			m_durationsTable.erase(i);
#endif
		}
	}

	return clockTicksDuration;
}



void
XSLTEngineImpl::displayDuration(
			const XalanDOMString&	info,
			const void*				key) const
{
	if(0 != key)
	{
		const clock_t	theDuration = popDuration(key);

		if(0 != m_diagnosticsPrintWriter)
		{
			const double	millis = (1000.0 * theDuration) / CLOCKS_PER_SEC;

			XalanDOMString	msg(info);

			msg += XALAN_STATIC_UCODE_STRING(" took ");
			msg += DoubleToDOMString(millis);
			msg += XALAN_STATIC_UCODE_STRING(" milliseconds");

			m_diagnosticsPrintWriter->println(msg);
		}
	}
}



void
XSLTEngineImpl::setDiagnosticsOutput(PrintWriter*	pw)
{
	m_diagnosticsPrintWriter = pw;

	m_problemListener->setPrintWriter(pw);
}



void
XSLTEngineImpl::diag(const XalanDOMString& 	s) const
{
	if (0 != m_diagnosticsPrintWriter)
	{
		m_diagnosticsPrintWriter->println(s);
	}
}



void
XSLTEngineImpl::setQuietConflictWarnings(bool	b)
{
	m_quietConflictWarnings = b;
}



void
XSLTEngineImpl::setDocumentLocator(const Locator* const		/* locator */)
{
	// Do nothing for now
}



void
XSLTEngineImpl::traceSelect(
			const XalanElement& 	theTemplate,
			const NodeRefListBase&	nl) const
{
	if (0 != m_diagnosticsPrintWriter)
	{
		XalanDOMString	msg = theTemplate.getNodeName() + XalanDOMString(XALAN_STATIC_UCODE_STRING(": "));

		XalanAttr*		attr = theTemplate.getAttributeNode(Constants::ATTRNAME_SELECT);

		if(0 != attr)
		{
			msg += attr->getValue();
			msg += XALAN_STATIC_UCODE_STRING(", ");
			msg += LongToDOMString(nl.getLength());
			msg += XALAN_STATIC_UCODE_STRING(" selected");
		}
		else
		{
			msg += XALAN_STATIC_UCODE_STRING("*|text(), (default select), ");
			msg += LongToDOMString(nl.getLength());
			msg += XALAN_STATIC_UCODE_STRING(" selected");
		}

		attr = theTemplate.getAttributeNode(Constants::ATTRNAME_MODE);

		if(0 != attr)
		{
			msg += XalanDOMString(XALAN_STATIC_UCODE_STRING(", mode = ")) + attr->getValue();
		}

		m_diagnosticsPrintWriter->println(msg);
	}
}



void
XSLTEngineImpl::startDocument()
{
	assert(m_flistener != 0);
	assert(m_executionContext != 0);

	if (m_hasPendingStartDocument == false)
	{
		m_hasPendingStartDocument = true;
		m_mustFlushStartDocument = false;
	}
	else if (m_mustFlushStartDocument == true)
	{
		m_flistener->startDocument();

		if(getTraceListeners() > 0)
		{
			GenerateEvent ge(this, GenerateEvent::EVENTTYPE_STARTDOCUMENT);

			fireGenerateEvent(ge);
		}

		// Reset this, but leave m_mustFlushStartDocument alone,
		// since it will still be needed.
		m_hasPendingStartDocument = false;
	}
}



void
XSLTEngineImpl::endDocument()
{
	assert(m_flistener != 0);
	assert(m_executionContext != 0);

	flushPending();

	m_flistener->endDocument();

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_ENDDOCUMENT);

		fireGenerateEvent(ge);
	}
}



void
XSLTEngineImpl::addResultNamespaceDecl(
			const XalanDOMString&	prefix, 
	        const XalanDOMString&	namespaceVal)
{
	const NameSpace		ns(prefix, namespaceVal);

	if (m_resultNameSpaces.size() == 0)
	{
		NamespaceVectorType		nsVector(1, ns);

		m_resultNameSpaces.push_back(nsVector);
	}
	else
	{
		NamespaceVectorType&	nsOnStack = m_resultNameSpaces.back();

		// If the last vector contains only an empty namespace, replace it with a
		// new vector containing only this namespace
		if(isEmpty(nsOnStack.front().getURI()))
		{
			nsOnStack.front() = ns;
		}
		// Otherwise, add the namespace at the end of the last vector
		else
		{
			nsOnStack.push_back(ns);
		}
	}
}



void
XSLTEngineImpl::addResultAttribute(
			AttributeListImpl&	attList,
			const XalanDOMString&	aname,
			const XalanDOMString&	value)
{
	const bool	isPrefix = startsWith(aname, DOMServices::s_XMLNamespaceWithSeparator);

	if (equals(aname, DOMServices::s_XMLNamespace) || isPrefix == true) 
	{
		const XalanDOMString	p = isPrefix == true ? substring(aname, 6) : XalanDOMString();

		addResultNamespaceDecl(p, value);
	}

	attList.removeAttribute(c_wstr(aname));

	if (length(value) > 0)
	{
		attList.addAttribute(
			c_wstr(aname),
			c_wstr(Constants::ATTRTYPE_CDATA),
			c_wstr(value));
	}
	else
	{
		const XalanDOMChar		theDummy = 0;

		attList.addAttribute(
			c_wstr(aname),
			c_wstr(Constants::ATTRTYPE_CDATA),
			&theDummy);
	}

}



bool
XSLTEngineImpl::pendingAttributesHasDefaultNS() const
{
	const unsigned int	n = m_pendingAttributes.getLength();

	for(unsigned int i = 0; i < n; i++)
	{
		if(equals(m_pendingAttributes.getName(i),
				  DOMServices::s_XMLNamespace) == true)
		{
			return true;
		}
	}

	return false;
}



void
XSLTEngineImpl::flushPending()
{
	if(m_hasPendingStartDocument == true && 0 != length(m_pendingElementName))
	{
		assert(m_flistener != 0);
		assert(m_executionContext != 0);

		if (m_stylesheetRoot->isOutputMethodSet() == false)
		{
			if (equalsIgnoreCase(m_pendingElementName,
								 Constants::ELEMNAME_HTML_STRING) == true &&
				pendingAttributesHasDefaultNS() == false)
			{
				if (m_flistener->getOutputFormat() == FormatterListener::OUTPUT_METHOD_XML)
				{
					// Yuck!!! Ugly hack to switch to HTML on-the-fly.
					FormatterToXML* const	theFormatter =
#if defined(XALAN_OLD_STYLE_CASTS)
						(FormatterToXML*)m_flistener;
#else
						static_cast<FormatterToXML*>(m_flistener);
#endif

					m_flistener =
						m_executionContext->createFormatterToHTML(
							theFormatter->getWriter(),
							theFormatter->getEncoding(),
							theFormatter->getMediaType(),
							theFormatter->getDoctypeSystem(),
							theFormatter->getDoctypePublic(),
							true,	// indent
							theFormatter->getIndent() > 0 ? theFormatter->getIndent() :
											StylesheetExecutionContext::eDefaultHTMLIndentAmount);
				}
			}
		}
	}

	if(m_hasPendingStartDocument == true && m_mustFlushStartDocument == true)
	{
		startDocument();
	}

	if(0 != length(m_pendingElementName) && m_mustFlushStartDocument == true)
	{
		assert(m_flistener != 0);
		assert(m_executionContext != 0);

		m_cdataStack.push_back(isCDataResultElem(m_pendingElementName)? true : false);
		m_flistener->startElement(c_wstr(m_pendingElementName), m_pendingAttributes);

		if(getTraceListeners() > 0)
		{
			const GenerateEvent	ge(this, GenerateEvent::EVENTTYPE_STARTELEMENT,
					m_pendingElementName, &m_pendingAttributes);

			fireGenerateEvent(ge);
		}

		m_pendingAttributes.clear();

		clear(m_pendingElementName);
	}
}



void
XSLTEngineImpl::startElement(const XMLCh* const	name)
{
	assert(m_flistener != 0);
	assert(name != 0);
	flushPending();

	// Push a new container on the stack, then push an empty
	// result namespace on to that container.
	NamespaceVectorType nsVector;
	nsVector.push_back(m_emptyNamespace);
	m_resultNameSpaces.push_back(nsVector);
	m_pendingElementName = name;

	m_mustFlushStartDocument = true;
}



void
XSLTEngineImpl::startElement(
			const XMLCh* const	name,
			AttributeList&		atts)
{
	assert(m_flistener != 0);
	assert(name != 0);

	flushPending();

	const unsigned int	nAtts = atts.getLength();

	m_pendingAttributes.clear();

	for(unsigned int i = 0; i < nAtts; i++)
	{
		m_pendingAttributes.addAttribute(atts.getName(i),
										 atts.getType(i),
										 atts.getValue(i));
	}

	// Push a new container on the stack, then push an empty
	// result namespace on to that container.
	NamespaceVectorType		nsVector;

	nsVector.push_back(m_emptyNamespace);

	m_resultNameSpaces.push_back(nsVector);

	m_pendingElementName = name;
}



void
XSLTEngineImpl::endElement(const XMLCh* const 	name)
{
	assert(m_flistener != 0);
	assert(name != 0);

	flushPending();

	m_flistener->endElement(name);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_ENDELEMENT, name, 0);
		fireGenerateEvent(ge);
	}

	m_resultNameSpaces.pop_back();

	const Stylesheet::QNameVectorType&	cdataElems =
		m_stylesheetRoot->getCDATASectionElems();

	if(0 != cdataElems.size())
	{
		m_cdataStack.pop_back();
	}
}


void
XSLTEngineImpl::characters(
			const XMLCh* const	ch,
			const unsigned int 	length)
{
	characters(ch,
			   0,
			   length);
}



void
XSLTEngineImpl::characters(
			const XMLCh* const	ch,
			const unsigned int	start,
			const unsigned int 	length)
{
	assert(m_flistener != 0);
	assert(ch != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	const Stylesheet::QNameVectorType&	cdataElems =
			m_stylesheetRoot->getCDATASectionElems();

	if(0 != cdataElems.size() && 0 != m_cdataStack.size())
	{
		m_flistener->cdata(ch + start, length);

		if(getTraceListeners() > 0)
		{
			GenerateEvent ge(this, GenerateEvent::EVENTTYPE_CDATA, ch, start, length);
			fireGenerateEvent(ge);
		}
	}
	else
	{
		m_flistener->characters(ch + start, length);

		if(getTraceListeners() > 0)
		{
			GenerateEvent ge(this, GenerateEvent::EVENTTYPE_CHARACTERS, ch,
						start, length);
			fireGenerateEvent(ge);
		}
	}
}




void 
XSLTEngineImpl::charactersRaw (
			const XMLCh* const	ch,
			const unsigned int	/* start */,
			const unsigned int	length)
{
	m_mustFlushStartDocument = true;

	flushPending();

	m_flistener->charactersRaw(ch, length);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_CHARACTERS,
				ch, 0, length);

		fireGenerateEvent(ge);
	}
}



void
XSLTEngineImpl::resetDocument()
{
	assert(m_flistener != 0);

	flushPending();
	
	m_flistener->resetDocument();
}



void
XSLTEngineImpl::ignorableWhitespace(
			const XMLCh* const	ch,
			const unsigned int 	length)
{
	assert(m_flistener != 0);
	assert(ch != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	m_flistener->ignorableWhitespace(ch, length);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_IGNORABLEWHITESPACE,
					ch, 0, length);

		fireGenerateEvent(ge);
	}
}



void
XSLTEngineImpl::processingInstruction(
			const XMLCh* const	target,
			const XMLCh* const	data)
{
	assert(m_flistener != 0);
	assert(target != 0);
	assert(data != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	m_flistener->processingInstruction(target, data);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_PI,
                                          target, data);

		fireGenerateEvent(ge);
	}
}



void
XSLTEngineImpl::comment(const XMLCh* const	data)
{
	assert(m_flistener != 0);
	assert(data != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	m_flistener->comment(data);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_COMMENT,
                                          data);
		fireGenerateEvent(ge);
	}
}


void
XSLTEngineImpl::entityReference(const XMLCh* const	name)
{
	assert(m_flistener != 0);
	assert(name != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	m_flistener->entityReference(name);

	if(getTraceListeners() > 0)
	{
		GenerateEvent ge(this, GenerateEvent::EVENTTYPE_ENTITYREF,
                                          name);

		fireGenerateEvent(ge);
	}
}



void
XSLTEngineImpl::cdata(
			const XMLCh* const	ch,
			const unsigned int 	start,
			const unsigned int 	length)
{
	assert(m_flistener != 0);
	assert(ch != 0);

	m_mustFlushStartDocument = true;

	flushPending();

	const Stylesheet::QNameVectorType&	cdataElems =
		m_stylesheetRoot->getCDATASectionElems();

	if(0 != cdataElems.size() && 0 != m_cdataStack.size())
	{
		m_flistener->cdata(ch, length);

		if(getTraceListeners() > 0)
		{
			GenerateEvent ge(this, GenerateEvent::EVENTTYPE_CDATA, ch, start,
					length);

			fireGenerateEvent(ge);
		}
	}
	else
	{
		m_flistener->characters(ch, length);

		if(getTraceListeners() > 0)
		{
			GenerateEvent ge(this, GenerateEvent::EVENTTYPE_CHARACTERS, ch,
					start, length);

			fireGenerateEvent(ge);
		}
	}
}



void
XSLTEngineImpl::cloneToResultTree(
			XalanNode&			node, 
			bool				isLiteral,
			bool				overrideStrip,
			bool				shouldCloneAttributes)
{
	bool	stripWhiteSpace = false;

	switch(node.getNodeType())
	{
	case XalanNode::TEXT_NODE:
		{
			// If stripWhiteSpace is false, then take this as an override and 
			// just preserve the space, otherwise use the XSL whitespace rules.
			if(!overrideStrip)
			{
				stripWhiteSpace = isLiteral ? true : false;
			  // was: stripWhiteSpace = isLiteral ? true : shouldStripSourceNode(node);
			}

			const XalanText& 	tx =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanText&)node;
#else
				static_cast<const XalanText&>(node);
#endif

			XalanDOMString		data;

			if(stripWhiteSpace == true)
			{
				if(tx.isIgnorableWhitespace())
				{
					data = getNormalizedText(tx);

					if((0 != length(data)) && (0 == length(trim(data))))
					{
						clear(data);
					}
				}
			}
			else 
			{
				XalanNode*	parent = node.getParentNode();

				if(parent == 0 || XalanNode::DOCUMENT_NODE != parent->getNodeType())
				{
					data = getNormalizedText(tx);
				}
			}
			

			if(0 != length(data))
			{
				// TODO: Hack around the issue of comments next to literals.
				// This would be, when a comment is present, the whitespace 
				// after the comment must be added to the literal.	The 
				// parser should do this, but XML4J doesn't seem to.
				// <foo>some lit text
				//	 <!-- comment -->	
				//	 </foo>
				// Loop through next siblings while they are comments, then, 
				// if the node after that is a ignorable text node, append 
				// it to the text node just added.
			  
				if(tx.isIgnorableWhitespace())
				{
					ignorableWhitespace(toCharArray(data), length(data));
				}
				else
				{
					characters(toCharArray(data), 0, length(data));
				}
			}
		}
		break;

	case XalanNode::ELEMENT_NODE:
		{
			if(shouldCloneAttributes == true)
			{
				copyAttributesToAttList(&node,
										m_stylesheetRoot,
#if defined(XALAN_OLD_STYLE_CASTS)
										(const XalanElement&)node,
#else
										static_cast<const XalanElement&>(node),
#endif
										m_pendingAttributes);

				copyNamespaceAttributes(node,
										false);
			}

			startElement(c_wstr(m_executionContext->getNameOfNode(node)));
		}
		break;

	case XalanNode::CDATA_SECTION_NODE:
		{
			const XalanCDATASection& 	theCDATA =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanCDATASection&)node;
#else
				static_cast<const XalanCDATASection&>(node);
#endif

			const XalanDOMString 	data = theCDATA.getData();

			cdata(toCharArray(data), 0, length(data));
		}
		break;

	case XalanNode::ATTRIBUTE_NODE:
		{
			const XalanAttr& 	attr =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanAttr&)node;
#else
				static_cast<const XalanAttr&>(node);
#endif
			addResultAttribute(m_pendingAttributes,
							   m_executionContext->getNameOfNode(attr),
							   attr.getValue());
		}
		break;

	case XalanNode::COMMENT_NODE:
		{
			const XalanComment&		theComment =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanComment&)node;
#else
				static_cast<const XalanComment&>(node);
#endif

			const XalanDOMString 	theData = theComment.getData();

			comment(toCharArray(theData));
		}
		break;

	case XalanNode::DOCUMENT_FRAGMENT_NODE:
		{
			error("No clone of a document fragment!");
		}
		break;
	
	case XalanNode::ENTITY_REFERENCE_NODE:
		{
			const XalanDOMString 	theName = node.getNodeName();
			entityReference(toCharArray(theName));
		}
		break;

	case XalanNode::PROCESSING_INSTRUCTION_NODE:
		{
			const XalanProcessingInstruction&	pi =
#if defined(XALAN_OLD_STYLE_CASTS)
				(const XalanProcessingInstruction&)node;
#else
				static_cast<const XalanProcessingInstruction&>(node);
#endif

			const XalanDOMString 	theTarget = pi.getTarget();
			const XalanDOMString 	theData = pi.getData();

			processingInstruction(toCharArray(theTarget),
								  toCharArray(theData));
		}
		break;

	// Can't really do this, but we won't throw an error so that copy-of will
	// work
	case XalanNode::DOCUMENT_NODE:
	break;

	default:
		error("Can not create item in result tree: " + node.getNodeName());
	break;

	}
}



ResultTreeFragBase*
XSLTEngineImpl::createResultTreeFrag(
			StylesheetExecutionContext&		executionContext,
			const ElemTemplateElement&		templateChild, 
			XalanNode*						sourceTree, 
			XalanNode*						sourceNode,
			const QName&					mode)
{
	XalanAutoPtr<ResultTreeFragBase> pfrag(createDocFrag());

	FormatterToDOM	tempFormatter(m_resultTreeFactory, 
								  pfrag.get(),
								  0);

	m_mustFlushStartDocument = true;

	flushPending();

	StylesheetExecutionContext::ExecutionStateSetAndRestore		theStateSaveAndRestore(
			executionContext,
			&tempFormatter);

	templateChild.executeChildren(executionContext, sourceTree, sourceNode, mode);

	return pfrag.release();
}



void
XSLTEngineImpl::outputResultTreeFragment(
			StylesheetExecutionContext&		executionContext,
			const XObject&					theTree)
{
	const ResultTreeFragBase&	docFrag = theTree.rtree(executionContext);

	const XalanNodeList*		nl = docFrag.getChildNodes();
	assert(nl != 0);

	const unsigned int			nChildren = nl->getLength();

	for(unsigned int i = 0; i < nChildren; i++)
	{
		XalanNode*			pos = nl->item(i);
		XalanNode* const	top = pos;

		while(0 != pos)
		{
			flushPending();

			cloneToResultTree(*pos, false, false, true);

			XalanNode*	nextNode = pos->getFirstChild();

			while(0 == nextNode)
			{
				if(XalanNode::ELEMENT_NODE == pos->getNodeType())
				{
					endElement(c_wstr(pos->getNodeName()));
				}

				if(top == pos)
					break;

				nextNode = pos->getNextSibling();

				if(0 == nextNode)
				{
					pos = pos->getParentNode();

					if(top == pos || 0 == pos)
					{
						if (0 != pos)
						{
							if(XalanNode::ELEMENT_NODE == pos->getNodeType())
							{
								endElement(c_wstr(pos->getNodeName()));
							}
						}

						nextNode = 0;
						break;
					}
				}
			}

			pos = nextNode;
		}
	}
}



bool
XSLTEngineImpl::isCDataResultElem(const XalanDOMString&		elementName) const
{
	typedef Stylesheet::QNameVectorType		QNameVectorType;

	bool is = false;
	const QNameVectorType&				cdataElems = m_stylesheetRoot->getCDATASectionElems();
	const QNameVectorType::size_type	theSize = cdataElems.size();

	if(0 != theSize)
	{
		XalanDOMString		elemNS;
		XalanDOMString		elemLocalName;

		const unsigned int	indexOfNSSep = indexOf(elementName, XalanUnicode::charColon);

		if(indexOfNSSep == length(elementName))
		{
			elemLocalName = elementName;
		}
		else
		{
			const XalanDOMString	prefix = substring(elementName, 0, indexOfNSSep);

			if(equals(prefix, DOMServices::s_XMLString))
			{
				elemNS = DOMServices::s_XMLNamespaceURI;
			}
			else
			{
				elemNS = getResultNamespaceForPrefix(prefix);
			}

			if(0 == length(elemNS))
			{
				error(XalanDOMString("Prefix must resolve to a namespace: ") + prefix);
			}

			elemLocalName = substring(elementName, indexOfNSSep + 1);
		}

		for(Stylesheet::QNameVectorType::size_type i = 0; i < theSize && is == false; i++)
		{
			const QName& qname = cdataElems[i];

			is = qname.equals(QName(elemNS, elemLocalName));
		}
	}

	return is;
}
	


// $$$ ToDo: This should not be here!!!!
bool
XSLTEngineImpl::qnameEqualsResultElemName(
			const QName&			qname,
			const XalanDOMString&	elementName) const
{
	XalanDOMString		elemNS;
	XalanDOMString		elemLocalName;

	const unsigned int	indexOfNSSep = indexOf(elementName, XalanUnicode::charColon);

	if(indexOfNSSep < length(elementName))
	{
		const XalanDOMString	prefix = substring(elementName, 0, indexOfNSSep);

		if(equals(prefix, DOMServices::s_XMLString))
		{
			elemNS = DOMServices::s_XMLNamespaceURI;
		}
		else
		{
			elemNS = getResultNamespaceForPrefix(prefix);
		}

		if(0 == elemNS.length())
		{
			error(XalanDOMString("Prefix must resolve to a namespace: ") + prefix);
		}

		elemLocalName =  substring(elementName, indexOfNSSep+1);
	}
	else
	{
		elemLocalName = elementName;
	}

	return qname.equals(QName(elemNS, elemLocalName));
}



const XalanDOMString&
XSLTEngineImpl::getResultNamespaceForPrefix(const XalanDOMString&	prefix) const
{
	// Search vector from first element back
	return QName::getNamespaceForPrefix(m_resultNameSpaces, prefix, false);
}
  


const XalanDOMString&
XSLTEngineImpl::getResultPrefixForNamespace(const XalanDOMString&	theNamespace) const
{
	// Search vector from first element back
	return QName::getPrefixForNamespace(m_resultNameSpaces, theNamespace, false);
}



XalanDOMString
XSLTEngineImpl::getPrefixForNamespace(
			const XalanDOMString&	theNamespace,
			const XalanElement&	namespaceContext) const
{
	XalanNode::NodeType		type;
	const XalanNode*		parent = &namespaceContext;
	XalanDOMString			prefix;

	while (0 != parent && 0 == length(prefix)
		   && ((type = parent->getNodeType()) == XalanNode::ELEMENT_NODE
				|| type == XalanNode::ENTITY_REFERENCE_NODE))
	{
		if (type == XalanNode::ELEMENT_NODE) 
		{
			const XalanNamedNodeMap* const	nnm =
				parent->getAttributes();
			assert(nnm != 0);

			const unsigned int	theLength = nnm->getLength();

			for (unsigned int i = 0; i < theLength;  i ++) 
			{
				const XalanNode* const	attr = nnm->item(i);
				assert(attr != 0);

				const XalanDOMString 	aname = attr->getNodeName();

				const bool				isPrefix = startsWith(aname, DOMServices::s_XMLNamespaceWithSeparator);

				if (equals(aname, DOMServices::s_XMLNamespace) || isPrefix) 
				{
					const unsigned int		index = indexOf(aname, XalanUnicode::charColon);
					assert(index < length(aname));

					const XalanDOMString 	namespaceOfPrefix = attr->getNodeValue();

					if((0 != length(namespaceOfPrefix)) &&
						equals(namespaceOfPrefix, theNamespace))
					{
						prefix = isPrefix ? substring(aname, index + 1) : XalanDOMString();
					}
				}
			}
		}

		parent = m_xpathSupport.getParentOfNode(*parent);
	}

	return prefix;
}



void
XSLTEngineImpl::copyNamespaceAttributes(
			const XalanNode&	src,
			bool				/* srcIsStylesheetTree */) 
{
	int type;

	const XalanNode*	parent = &src;

	while (parent != 0
		   && ((type = parent->getNodeType()) == XalanNode::ELEMENT_NODE
			   || type == XalanNode::ENTITY_REFERENCE_NODE)) 
	{
		if (type == XalanNode::ELEMENT_NODE) 
		{
			const XalanNamedNodeMap* const	nnm =
				parent->getAttributes();
			assert(nnm != 0);

			const unsigned int	nAttrs = nnm->getLength();

			for (unsigned int i = 0;  i < nAttrs; i++) 
			{
				const XalanNode* const	attr = nnm->item(i);

				const XalanDOMString 	aname = attr->getNodeName();

				const bool				isPrefix = startsWith(aname, DOMServices::s_XMLNamespaceWithSeparator);

				if (equals(aname, DOMServices::s_XMLNamespace) || isPrefix) 
				{
					const XalanDOMString 	prefix = isPrefix ? substring(aname, 6) : XalanDOMString();
					const XalanDOMString 	desturi = getResultNamespaceForPrefix(prefix);
					XalanDOMString			srcURI = attr->getNodeValue();
					/*
					@@ JMD: Not used anymore in java ...
					const bool			isXSLNS =
						srcIsStylesheetTree && equalsIgnoreCase(srcURI, s_XSLNameSpaceURL)
						|| 0 != m_stylesheetRoot->lookupExtensionNSHandler(srcURI)
						|| srcIsStylesheetTree && equalsIgnoreCase(srcURI, s_XSLT4JNameSpaceURL);

					if(startsWith(srcURI, XALAN_STATIC_UCODE_STRING("quote:")))
					{
						srcURI = substring(srcURI, 6);
					}

					if(!equalsIgnoreCase(srcURI, desturi) && !isXSLNS)
					*/
					if(!equalsIgnoreCase(srcURI, desturi))
					{
						addResultAttribute(m_pendingAttributes, aname, srcURI);
					}
				}
			}
		}

		parent = parent->getParentNode();
	}
}



const XObject*
XSLTEngineImpl::evalXPathStr(
			const XalanDOMString&	str,
			XPathExecutionContext&	executionContext)
{
	XPath* const	theXPath = m_xpathFactory.create();

	XPathGuard	theGuard(m_xpathFactory,
						 theXPath);

    m_xpathProcessor->initXPath(*theXPath,
								str,
								*executionContext.getPrefixResolver(),
								m_xpathEnvSupport);

    return theXPath->execute(executionContext.getCurrentNode(),
							 *executionContext.getPrefixResolver(),
							 executionContext);
}



const XObject*
XSLTEngineImpl::evalXPathStr(
			const XalanDOMString&	str,
			XalanNode*				contextNode,
			const PrefixResolver&	prefixResolver,
			XPathExecutionContext&	executionContext)
{
	XPath* const	theXPath = m_xpathFactory.create();

	XPathGuard	theGuard(m_xpathFactory,
						 theXPath);

    m_xpathProcessor->initXPath(*theXPath,
								str,
								prefixResolver,
								m_xpathEnvSupport);

    return theXPath->execute(contextNode, prefixResolver, executionContext);
}



const XObject*
XSLTEngineImpl::evalXPathStr(
			const XalanDOMString&	str,
			XalanNode*				contextNode,
			const XalanElement&		prefixResolver,
			XPathExecutionContext&	executionContext)
{
	ElementPrefixResolverProxy	theProxy(&prefixResolver,
										 m_xpathEnvSupport,
										 m_xpathSupport);

	return evalXPathStr(str, contextNode, theProxy, executionContext);
}




/**
 * Create and initialize an xpath and return it.
 */
const XPath*
XSLTEngineImpl::createMatchPattern(
			const XalanDOMString&	str,
			const PrefixResolver&	resolver)
{
	XPath* const	xpath = m_xpathFactory.create();

	m_xpathProcessor->initMatchPattern(*xpath, str, resolver, m_xpathEnvSupport);

	return xpath;
}



void
XSLTEngineImpl::returnXPath(const XPath*	xpath)
{
	m_xpathFactory.returnObject(xpath);
}



XalanDOMString
XSLTEngineImpl::getAttrVal(
			const XalanElement& 	el,
			const XalanDOMString&	key,
			const XalanNode&		/* contextNode */		)
{
	return getAttrVal(el, key);
}



XalanDOMString
XSLTEngineImpl::getAttrVal(
			const XalanElement&		el,
			const XalanDOMString&	key)
{
	const XalanAttr* const	a = el.getAttributeNode(key);

	return 0 == a ? XalanDOMString() : a->getValue();
}



static const XalanDOMChar	theTokenDelimiterCharacters[] =
{
		XalanUnicode::charLeftCurlyBracket,
		XalanUnicode::charRightCurlyBracket,
		XalanUnicode::charApostrophe,
		XalanUnicode::charQuoteMark,
		0
};



static const XalanDOMChar	theLeftCurlyBracketString[] =
{
		XalanUnicode::charLeftCurlyBracket,
		0
};



static const XalanDOMChar	theRightCurlyBracketString[] =
{
		XalanUnicode::charRightCurlyBracket,
		0
};



// $$$ ToDo: Get rid of this!!! See ElemPI, ElemSort, etc.  These have strings instead of
// AVT instances...
XalanDOMString
XSLTEngineImpl::evaluateAttrVal(
			XalanNode*				contextNode,
			const PrefixResolver&	namespaceContext,
			const XalanDOMString&	stringedValue,
			XPathExecutionContext&	executionContext)
{
	XalanDOMString		expressedValue;

	StringTokenizer 	tokenizer(stringedValue, theTokenDelimiterCharacters, true);

	const unsigned int	nTokens = tokenizer.countTokens();

	if(nTokens < 2)
	{
		expressedValue = stringedValue; // then do the simple thing
	}
	else
	{
		XalanDOMString	buffer;
		XalanDOMString	t; // base token
		XalanDOMString	lookahead; // next token
		XalanDOMString	error; // if not empty, break from loop

		while(tokenizer.hasMoreTokens())
		{
			if(length(lookahead) != 0)
			{
				t = lookahead;
				clear(lookahead);
			}
			else t = tokenizer.nextToken();

			if(length(t) == 1)
			{
				switch(charAt(t, 0))
				{
					case XalanUnicode::charApostrophe:
					case XalanUnicode::charQuoteMark:
					{
						// just keep on going, since we're not in an attribute template
						append(buffer, t);
						break;
					}
					case(XalanUnicode::charLeftCurlyBracket):
					{
						// Attr template start
						lookahead = tokenizer.nextToken();
						if(equals(lookahead, theLeftCurlyBracketString))
						{
							// Double curlys mean escape to show curly
							append(buffer, lookahead);
							clear(lookahead);
							break; // from switch
						}
						/*
						else if(equals(lookahead, XalanUnicode::charQuoteMar) ||
								equals(lookahead, XalanUnicode::charApostrophe))
						{
							// Error. Expressions can not begin with quotes.
							error = "Expressions can not begin with quotes.";
							break; // from switch
						}
						*/
						else
						{
							XalanDOMString expression = lookahead; // Probably should make into StringBuffer

							while(0 != length(lookahead) && !equals(lookahead, theRightCurlyBracketString))
							{
								lookahead = tokenizer.nextToken();
								if(length(lookahead) == 1)
								{
									switch(charAt(lookahead, 0))
									{
										case XalanUnicode::charApostrophe:
										case XalanUnicode::charQuoteMark:
										{
											// String start
											expression += lookahead;
											XalanDOMString	quote = lookahead;
											// Consume stuff 'till next quote
											lookahead = tokenizer.nextToken();
											while(!equals(lookahead, quote))
											{
												expression += lookahead;
												lookahead = tokenizer.nextToken();
											}
											expression += lookahead;
											break;
										}
										case XalanUnicode::charLeftCurlyBracket:
										{
											// What's another curly doing here?
											error = "Error: Can not have \"{\" within expression.";
											break;
										}
										case XalanUnicode::charRightCurlyBracket:
										{
											// Proper close of attribute template.
											// Evaluate the expression.
											const XObject* const	xobj =
												evalXPathStr(expression, contextNode, namespaceContext, executionContext);

											const XalanDOMString			exprResult(xobj->str());

											append(buffer, exprResult);

											clear(lookahead); // breaks out of inner while loop
										break;
										}
										default:
										{
											// part of the template stuff, just add it.
											expression += lookahead;
										}
									} // end inner switch
								} // end if lookahead length == 1
								else
								{
									// part of the template stuff, just add it.
									expression += lookahead;
								}
							} // (0 != length(lookahead) && !equals(lookahead, "}"))

							if(length(error) != 0)
							{
								break; // from inner while loop
							}
						}
						break;
					}
					case(XalanUnicode::charRightCurlyBracket):
					{
						lookahead = tokenizer.nextToken();
						if(equals(lookahead, theRightCurlyBracketString))
						{
							// Double curlys mean escape to show curly
							append(buffer, lookahead);
							clear(lookahead); // swallow
						}
						else
						{
							// Illegal, I think...
							warn("Found \"}\" but no attribute template open!");
							append(buffer, theRightCurlyBracketString);
							// leave the lookahead to be processed by the next round.
						}
						break;
					}
					default:
					{
						// Anything else just add to string.
						append(buffer, t);
					}
				} // end switch t
			} // end if length == 1
			else
			{
				// Anything else just add to string.
				append(buffer, t);
			}

			if(0 != length(error))
			{
				// $$$ ToDo: Fix this when XalanDOMString::operator+() is const.
				XalanDOMString	message("Attr Template, ");

				warn(message + error);
				break;
			}
		} // end while(tokenizer.hasMoreTokens())

		expressedValue = buffer;
	} // end else nTokens > 1

	return expressedValue;
}



void
XSLTEngineImpl::copyAttributeToTarget(
			const XalanAttr&		attr,
			XalanNode*				/* contextNode */,
			const Stylesheet* 		/* stylesheetTree */,
			AttributeListImpl&		attrList, 
			const XalanElement& 	/* namespaceContext */)
{
	const XalanDOMString 	attrName = trim(attr.getName());

	XalanDOMString			stringedValue = attr.getValue();
//	stringedValue = evaluateAttrVal(contextNode,
//									namespaceContext,
//									stringedValue);

	// evaluateAttrVal might return a null value if the template expression 
	// did not turn up a result, in which case I'm going to not add the 
	// attribute.
	// TODO: Find out about empty attribute template expression handling.
	if(0 != length(stringedValue))
	{
		if((equals(attrName, DOMServices::s_XMLNamespace) || startsWith(attrName, DOMServices::s_XMLNamespaceWithSeparator))
		   && startsWith(stringedValue, XALAN_STATIC_UCODE_STRING("quote:")))
		{
			stringedValue = substring(stringedValue, 6);
		}

		addResultAttribute(attrList, attrName, stringedValue);
	}
}



void
XSLTEngineImpl::copyAttributesToAttList(
			XalanNode*				contextNode,
			const Stylesheet* 		stylesheetTree,
			const XalanElement& 	templateChild,
			AttributeListImpl&		attList)
{
	assert(m_stylesheetRoot != 0);
	assert(stylesheetTree != 0);
	const XalanNamedNodeMap* const	attributes =
		templateChild.getAttributes();

	const unsigned int	nAttributes = (0 != attributes) ? attributes->getLength() : 0;

	XalanDOMString	attrSetUseVal;

	for(unsigned int	i = 0; i < nAttributes; i++)  
	{	
		const XalanAttr* const 	attr =
#if defined(XALAN_OLD_STYLE_CASTS)
			(const XalanAttr*)attributes->item(i);
#else
			static_cast<const XalanAttr*>(attributes->item(i));
#endif
		assert(attr != 0);

		const XalanDOMString	theTemp(s_XSLNameSpaceURL + ":use");

		if(equalsIgnoreCase(m_parserLiaison.getExpandedAttributeName(*attr), theTemp))
		{
			attrSetUseVal = attr->getValue();
		}
		else
		{
			copyAttributeToTarget(*attr,
								  contextNode,
								  stylesheetTree, 
								  attList,
								  templateChild);
		}
	}
}


 
XalanElement*
XSLTEngineImpl::getElementByID(
			const XalanDOMString&	id,
			const XalanDocument&	doc) const
{
	return m_xpathSupport.getElementByID(id, doc);
}



bool
XSLTEngineImpl::shouldStripSourceNode(
			XPathExecutionContext&	executionContext,
			const XalanNode&		textNode) const
{
	assert(m_stylesheetRoot != 0);

	bool	strip = false; // return value

	if((m_stylesheetRoot->getWhitespacePreservingElements().size() > 0 ||
	    m_stylesheetRoot->getWhitespaceStrippingElements().size() > 0))
	{
		const XalanNode::NodeType	type = textNode.getNodeType();
		if((XalanNode::TEXT_NODE == type) || (XalanNode::CDATA_SECTION_NODE == type))
		{
			const XalanText& 	theTextNode =
#if defined(XALAN_OLD_STYLE_CASTS)
					(const XalanText&)textNode;
#else
					static_cast<const XalanText&>(textNode);
#endif

			if(!theTextNode.isIgnorableWhitespace())
			{
				const XalanDOMString	data = theTextNode.getData();

				if(0 == length(data))
				{
					return true;
				}
				else if(!isWhiteSpace(data))
				{
					return false;
				}
			}

			XalanNode*	parent = m_xpathSupport.getParentOfNode(textNode);

			while(0 != parent)
			{
				if(parent->getNodeType() == XalanNode::ELEMENT_NODE)
				{
					const XalanElement*	const	parentElem =
#if defined(XALAN_OLD_STYLE_CASTS)
						(const XalanElement*)parent;
#else
						static_cast<const XalanElement*>(parent);
#endif

					double highPreserveScore = XPath::s_MatchScoreNone;
					double highStripScore = XPath::s_MatchScoreNone;

					ElementPrefixResolverProxy		theProxy(parentElem, m_xpathEnvSupport, m_xpathSupport);

					{
						// $$$ ToDo:  All of this should be moved into a member of
						// Stylesheet, so as not to expose these two data members...
						typedef Stylesheet::XPathVectorType		XPathVectorType;

						const XPathVectorType&	theElements =
							m_stylesheetRoot->getWhitespacePreservingElements();

						const XPathVectorType::size_type	nTests =
							theElements.size();

						for(XPathVectorType::size_type i = 0; i < nTests; i++)
						{
							const XPath* const	matchPat = theElements[i];
							assert(matchPat != 0);

							const double	score = matchPat->getMatchScore(parent, theProxy, executionContext);

							if(score > highPreserveScore)
								highPreserveScore = score;
						}
					}

					{
						typedef Stylesheet::XPathVectorType		XPathVectorType;

						const XPathVectorType&	theElements =
							m_stylesheetRoot->getWhitespaceStrippingElements();

						const XPathVectorType::size_type	nTests =
							theElements.size();

						for(XPathVectorType::size_type i = 0; i < nTests; i++)
						{
							const XPath* const	matchPat =
								theElements[i];
							assert(matchPat != 0);

							const double	score = matchPat->getMatchScore(parent, theProxy, executionContext);

							if(score > highStripScore)
								highStripScore = score;
						}
					}

					if(highPreserveScore > XPath::s_MatchScoreNone ||
					   highStripScore > XPath::s_MatchScoreNone)
					{
						if(highPreserveScore > highStripScore)
						{
							strip = false;
						}
						else if(highStripScore > highPreserveScore)
						{
							strip = true;
						}
						else
						{
							warn("Match conflict between xsl:strip-space and xsl:preserve-space");
						}
						break;
					}
				}

				parent = parent->getParentNode();
			}
		}
	}

	return strip;
}



XalanDOMString
XSLTEngineImpl::fixWhiteSpace(
			const XalanDOMString&	string, 
			bool					trimHead, 
			bool					trimTail, 
			bool					doublePunctuationSpaces) 
{
	const XalanDOMChar* const	theStringData = c_wstr(string);


	XalanDOMCharVectorType		buf(
					theStringData,
					theStringData + length(string));

	const unsigned int	len = buf.size();

	bool				edit = false;

	unsigned int 		s;

	for(s = 0;	s < len;  ++s) 
	{
		if(isSpace(buf[s]) == true) 
		{
			break;
		}
	}

	/* replace S to ' '. and ' '+ -> single ' '. */
	unsigned int	d = s;

	bool			pres = false;

	for ( ;  s < len;  ++s)
	{
		const XalanDOMChar 	c = buf[s];

		if (isSpace(c) == true) 
		{
			if (!pres) 
			{
				if (XalanUnicode::charSpace != c)  
				{
					edit = true;
				}

				buf[d++] = XalanUnicode::charSpace;

				if(doublePunctuationSpaces == true && (s != 0))
				{
					const XalanDOMChar 	prevChar = buf[s - 1];

					if(!(prevChar == XalanUnicode::charFullStop ||
						 prevChar == XalanUnicode::charExclamationMark ||
						 prevChar == XalanUnicode::charQuestionMark))
					{
						pres = true;
					}
				}
				else
				{
					pres = true;
				}
			}
			else
			{
				edit = true;
				pres = true;
			}
		}
		else 
		{
			buf[d++] = c;
			pres = false;
		}
	}

	if (trimTail == true && 1 <= d && XalanUnicode::charSpace == buf[d - 1]) 
	{
		edit = true;
		d --;
	}

	XalanDOMCharVectorType::const_iterator	start = buf.begin();

	if (trimHead  == true && 0 < d && XalanUnicode::charSpace == buf[0]) 
	{
		edit = true;
		start++;
	}

	if (edit == false)
	{
		// If we haven't changed the string, just return a copy of the
		// input string.
		return string;
	}
	else
	{
		// OK, we have to calculate the length of the string,
		// taking into account that we may have moved up the
		// start because we're trimming the from of the string.
		const unsigned int	theLength = d - (start - buf.begin());

		return XalanDOMString(start, theLength);
	}
}



const XalanDOMString
XSLTEngineImpl::getNormalizedText(const XalanText&	tx) const
{
	if(m_outputCarriageReturns == false && m_outputLinefeeds == false)
	{
		return tx.getData();
	}

	const XalanDOMString 	src = tx.getData();

	const int				nSrcChars = src.length();

	XalanDOMCharVectorType	sb;

	XalanDOMChar			prevChar = 0;

	for(int i = 0; i < nSrcChars; i++)
	{
		const XalanDOMChar	c = charAt(src, i);

		if(0x0A == c)
		{
			if(0x0D != prevChar)
			{
				if(m_outputCarriageReturns == true)
					sb.push_back(0x0D);
				if(m_outputLinefeeds == true)
					sb.push_back(0x0A);
			}
		}
		else if(0x0D == c)
		{
			if(m_outputCarriageReturns == true)
				sb.push_back(0x0D);
			if(m_outputLinefeeds == true)
				sb.push_back(0x0A);
		}
		else
		{
			sb.push_back(c);
		}
		prevChar = c;
	}

	sb.push_back(0);	// Null terminate

	return XalanDOMString(sb.begin(), sb.size());
}



XMLParserLiaison&
XSLTEngineImpl::getXMLParserLiaison() const
{
	return m_parserLiaison;
}



const XalanDOMString
XSLTEngineImpl::getUniqueNSValue() const
{
#if defined(XALAN_NO_MUTABLE)
	const unsigned long		temp = m_uniqueNSValue;

	((XSLTEngineImpl*)this)->m_uniqueNSValue++;

	return s_uniqueNamespacePrefix + UnsignedLongToDOMString(temp);
#else
	return s_uniqueNamespacePrefix + UnsignedLongToDOMString(m_uniqueNSValue++);
#endif
}



XalanDocument*
XSLTEngineImpl::getDOMFactory() const
{
	if(m_resultTreeFactory == 0)
	{
#if defined(XALAN_NO_MUTABLE)
		((XSLTEngineImpl*)this)->m_resultTreeFactory = m_parserLiaison.createDocument();
#else
		m_resultTreeFactory = m_parserLiaison.createDocument();
#endif
	}

	return m_resultTreeFactory;
}



/**
 * Create a document fragment.  This function may return null.
 */
ResultTreeFragBase* XSLTEngineImpl::createDocFrag() const
{
	return new ResultTreeFrag(*getDOMFactory());
}
  


XLocator*
XSLTEngineImpl::getXLocatorFromNode(const XalanNode*	node) const
{
	return m_xpathEnvSupport.getXLocatorFromNode(node);
}
	


void
XSLTEngineImpl::associateXLocatorToNode(
			const XalanNode*	node,
			XLocator*			xlocator)
{
	m_xpathEnvSupport.associateXLocatorToNode(node, xlocator);
}



ResultTreeFragBase*
XSLTEngineImpl::createResultTreeFrag() const
{
	return new ResultTreeFrag(*getDOMFactory());
}



void
XSLTEngineImpl::setStylesheetParam(
			const XalanDOMString&	theName,
			const XalanDOMString&	expression)
{
	const QName		qname(theName, 0, m_xpathEnvSupport, m_xpathSupport);

	m_topLevelParams.push_back(ParamVectorType::value_type(qname, expression));
}



void
XSLTEngineImpl::setStylesheetParam(
			const XalanDOMString&	theName,
			XObject*				theValue)
{
	const QName		qname(theName, 0, m_xpathEnvSupport, m_xpathSupport);

	m_topLevelParams.push_back(ParamVectorType::value_type(qname, theValue));
}



void
XSLTEngineImpl::resolveTopLevelParams(StylesheetExecutionContext&	executionContext)
{
	executionContext.pushTopLevelVariables(m_topLevelParams);
}



void
XSLTEngineImpl::resetCurrentState(
			XalanNode*	/* sourceTree */,
			XalanNode*	xmlNode)
{
	if(0 != xmlNode)
	{
		//===============================================
		// This will be used with callbacks from script, 
		// in places like getAttributeCallback.
		m_currentNode = xmlNode;
	}
}



// $$$ ToDo: This really belongs in DOMServices or DOMSupport()
XalanElement*
XSLTEngineImpl::findElementByAttribute(
			XalanElement& 			elem,
			const XalanDOMString&	targetElementName, 
			const XalanDOMString&	targetAttributeName,
			const XalanDOMString&	targetAttributeValue)
{
	XalanElement* 			theFoundElement = 0;

	const XalanDOMString 	tagName = elem.getTagName();

	if(0 == length(targetElementName) || equals(tagName, targetElementName))
	{
		const XalanNamedNodeMap* const	attributes = elem.getAttributes();

		try
		{
			const unsigned int	nAttributes = 0 != attributes ? attributes->getLength() : 0;

			for(unsigned int i = 0; i < nAttributes; i++)  
			{
				const XalanAttr* 		attr =
#if defined(XALAN_OLD_STYLE_CASTS)
						  (const XalanAttr*)attributes->item(i);
#else
						  static_cast<const XalanAttr*>(attributes->item(i));
#endif

				const XalanDOMString 	attrName = attr->getName();

				if(equals(attrName, targetAttributeName))
				{
					const XalanDOMString	attrVal = attr->getValue();

					if(equals(attrVal, targetAttributeValue))
					{
						theFoundElement = &elem;
						break;
					}
				}
			}
		}
		catch(const XalanDOMException&)
		{
		}
	}

	if(0 == theFoundElement)
	{
		XalanNode*	childNode = elem.getFirstChild();

		while(childNode != 0) 
		{
			if (childNode->getNodeType() == XalanNode::ELEMENT_NODE) 
			{
				XalanElement*	child = 
#if defined(XALAN_OLD_STYLE_CASTS)
						  (XalanElement*)childNode;
#else
						  static_cast<XalanElement*>(childNode);
#endif

				const XalanDOMString 	childName = child->getTagName();

				if(0 != length(childName))
				{
					theFoundElement = findElementByAttribute(
													 *child,
													 targetElementName, 
													 targetAttributeName,
													 targetAttributeValue);

					if(0 != theFoundElement)
					{
						break;
					}
				}
			}

			childNode = childNode->getNextSibling();
		}
	}

	return theFoundElement;
}



FormatterListener*
XSLTEngineImpl::getFormatterListener() const
{
	return m_flistener;
}



void
XSLTEngineImpl::setFormatterListener(FormatterListener*		flistener)
{
	if (m_hasPendingStartDocument == true && m_flistener != 0)
	{
		m_mustFlushStartDocument = true;

		flushPending();
	}

	m_flistener = flistener;
}



void
XSLTEngineImpl::installFunctions()
{
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("current"), FunctionCurrent());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("document"), FunctionDocument());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("element-available"), FunctionElementAvailable());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("function-available"), FunctionFunctionAvailable());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("format-number"), FunctionFormatNumber());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("generate-id"), FunctionGenerateID());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("key"), FunctionKey());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("system-property"), FunctionSystemProperty());
	XPath::installFunction(XALAN_STATIC_UCODE_STRING("unparsed-entity-uri"), FunctionUnparsedEntityURI());
}



void
XSLTEngineImpl::uninstallFunctions()
{
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("current"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("document"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("element-available"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("function-available"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("format-number"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("generate-id"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("key"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("system-property"));
	XPath::uninstallFunction(XALAN_STATIC_UCODE_STRING("unparsed-entity-uri"));
}



void
XSLTEngineImpl::initializeAttributeKeysTable(AttributeKeysMapType&	theAttributeKeys)
{
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_METHOD] = Constants::TATTRNAME_OUTPUT_METHOD;
	theAttributeKeys[Constants::ATTRNAME_AMOUNT] = Constants::TATTRNAME_AMOUNT;
	theAttributeKeys[Constants::ATTRNAME_ANCESTOR] = Constants::TATTRNAME_ANCESTOR;
	theAttributeKeys[Constants::ATTRNAME_ARCHIVE] = Constants::TATTRNAME_ARCHIVE;
	theAttributeKeys[Constants::ATTRNAME_ATTRIBUTE] = Constants::TATTRNAME_ATTRIBUTE;
	theAttributeKeys[Constants::ATTRNAME_ATTRIBUTE_SET] = Constants::TATTRNAME_ATTRIBUTE_SET;
	theAttributeKeys[Constants::ATTRNAME_CASEORDER] = Constants::TATTRNAME_CASEORDER;
	theAttributeKeys[Constants::ATTRNAME_CLASS] = Constants::TATTRNAME_CLASS;
	theAttributeKeys[Constants::ATTRNAME_CLASSID] = Constants::TATTRNAME_CLASSID;
	theAttributeKeys[Constants::ATTRNAME_CODEBASE] = Constants::TATTRNAME_CODEBASE;
	theAttributeKeys[Constants::ATTRNAME_CODETYPE] = Constants::TATTRNAME_CODETYPE;
	theAttributeKeys[Constants::ATTRNAME_CONDITION] = Constants::TATTRNAME_CONDITION;
	theAttributeKeys[Constants::ATTRNAME_COPYTYPE] = Constants::TATTRNAME_COPYTYPE;
	theAttributeKeys[Constants::ATTRNAME_COUNT] = Constants::TATTRNAME_COUNT;
	theAttributeKeys[Constants::ATTRNAME_DATATYPE] = Constants::TATTRNAME_DATATYPE;
	theAttributeKeys[Constants::ATTRNAME_DEFAULT] = Constants::TATTRNAME_DEFAULT;
	theAttributeKeys[Constants::ATTRNAME_DEFAULTSPACE] = Constants::TATTRNAME_DEFAULTSPACE;
	theAttributeKeys[Constants::ATTRNAME_DEPTH] = Constants::TATTRNAME_DEPTH;
	theAttributeKeys[Constants::ATTRNAME_DIGITGROUPSEP] = Constants::TATTRNAME_DIGITGROUPSEP;
	theAttributeKeys[Constants::ATTRNAME_DISABLE_OUTPUT_ESCAPING] = Constants::TATTRNAME_DISABLE_OUTPUT_ESCAPING;
	theAttributeKeys[Constants::ATTRNAME_ELEMENT] = Constants::TATTRNAME_ELEMENT;
	theAttributeKeys[Constants::ATTRNAME_ELEMENTS] = Constants::TATTRNAME_ELEMENTS;
	theAttributeKeys[Constants::ATTRNAME_EXPR] = Constants::TATTRNAME_EXPR;
	theAttributeKeys[Constants::ATTRNAME_EXTENSIONELEMENTPREFIXES] = Constants::TATTRNAME_EXTENSIONELEMENTPREFIXES;
	theAttributeKeys[Constants::ATTRNAME_FORMAT] = Constants::TATTRNAME_FORMAT;
	theAttributeKeys[Constants::ATTRNAME_FROM] = Constants::TATTRNAME_FROM;
	theAttributeKeys[Constants::ATTRNAME_GROUPINGSEPARATOR] = Constants::TATTRNAME_GROUPINGSEPARATOR;
	theAttributeKeys[Constants::ATTRNAME_GROUPINGSIZE] = Constants::TATTRNAME_GROUPINGSIZE;
	theAttributeKeys[Constants::ATTRNAME_HREF] = Constants::TATTRNAME_HREF;
	theAttributeKeys[Constants::ATTRNAME_ID] = Constants::TATTRNAME_ID;
	theAttributeKeys[Constants::ATTRNAME_IMPORTANCE] = Constants::TATTRNAME_IMPORTANCE;
	theAttributeKeys[Constants::ATTRNAME_INDENTRESULT] = Constants::TATTRNAME_INDENTRESULT;
	theAttributeKeys[Constants::ATTRNAME_LANG] = Constants::TATTRNAME_LANG;
	theAttributeKeys[Constants::ATTRNAME_LETTERVALUE] = Constants::TATTRNAME_LETTERVALUE;
	theAttributeKeys[Constants::ATTRNAME_LEVEL] = Constants::TATTRNAME_LEVEL;
	theAttributeKeys[Constants::ATTRNAME_MATCH] = Constants::TATTRNAME_MATCH;
	theAttributeKeys[Constants::ATTRNAME_METHOD] = Constants::TATTRNAME_METHOD;
	theAttributeKeys[Constants::ATTRNAME_MODE] = Constants::TATTRNAME_MODE;
	theAttributeKeys[Constants::ATTRNAME_NAME] = Constants::TATTRNAME_NAME;
	theAttributeKeys[Constants::ATTRNAME_NAMESPACE] = Constants::TATTRNAME_NAMESPACE;
	theAttributeKeys[Constants::ATTRNAME_NDIGITSPERGROUP] = Constants::TATTRNAME_NDIGITSPERGROUP;
	theAttributeKeys[Constants::ATTRNAME_NS] = Constants::TATTRNAME_NS;
	theAttributeKeys[Constants::ATTRNAME_ONLY] = Constants::TATTRNAME_ONLY;
	theAttributeKeys[Constants::ATTRNAME_ORDER] = Constants::TATTRNAME_ORDER;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_CDATA_SECTION_ELEMENTS] = Constants::TATTRNAME_OUTPUT_CDATA_SECTION_ELEMENTS;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_DOCTYPE_PUBLIC] = Constants::TATTRNAME_OUTPUT_DOCTYPE_PUBLIC;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_DOCTYPE_SYSTEM] = Constants::TATTRNAME_OUTPUT_DOCTYPE_SYSTEM;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_ENCODING] = Constants::TATTRNAME_OUTPUT_ENCODING;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_INDENT] = Constants::TATTRNAME_OUTPUT_INDENT;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_MEDIATYPE] = Constants::TATTRNAME_OUTPUT_MEDIATYPE;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_STANDALONE] = Constants::TATTRNAME_OUTPUT_STANDALONE;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_VERSION] = Constants::TATTRNAME_OUTPUT_VERSION;
	theAttributeKeys[Constants::ATTRNAME_OUTPUT_OMITXMLDECL] = Constants::TATTRNAME_OUTPUT_OMITXMLDECL;
	theAttributeKeys[Constants::ATTRNAME_PRIORITY] = Constants::TATTRNAME_PRIORITY;
	theAttributeKeys[Constants::ATTRNAME_REFID] = Constants::TATTRNAME_REFID;
	theAttributeKeys[Constants::ATTRNAME_RESULTNS] = Constants::TATTRNAME_RESULTNS;
	theAttributeKeys[Constants::ATTRNAME_SELECT] = Constants::TATTRNAME_SELECT;
	theAttributeKeys[Constants::ATTRNAME_SEQUENCESRC] = Constants::TATTRNAME_SEQUENCESRC;
	theAttributeKeys[Constants::ATTRNAME_STYLE] = Constants::TATTRNAME_STYLE;
	theAttributeKeys[Constants::ATTRNAME_TEST] = Constants::TATTRNAME_TEST;
	theAttributeKeys[Constants::ATTRNAME_TOSTRING] = Constants::TATTRNAME_TOSTRING;
	theAttributeKeys[Constants::ATTRNAME_TYPE] = Constants::TATTRNAME_TYPE;
	theAttributeKeys[Constants::ATTRNAME_USE] = Constants::TATTRNAME_USE;
	theAttributeKeys[Constants::ATTRNAME_USEATTRIBUTESETS] = Constants::TATTRNAME_USEATTRIBUTESETS;
	theAttributeKeys[Constants::ATTRNAME_VALUE] = Constants::TATTRNAME_VALUE;

	theAttributeKeys[Constants::ATTRNAME_XMLNSDEF] = Constants::TATTRNAME_XMLNSDEF;
	theAttributeKeys[Constants::ATTRNAME_XMLNS] = Constants::TATTRNAME_XMLNS;
	theAttributeKeys[Constants::ATTRNAME_XMLSPACE] = Constants::TATTRNAME_XMLSPACE;
}



void
XSLTEngineImpl::initializeElementKeysTable(ElementKeysMapType&	theElementKeys)
{
	theElementKeys[Constants::ELEMNAME_APPLY_TEMPLATES_STRING] = Constants::ELEMNAME_APPLY_TEMPLATES;
	theElementKeys[Constants::ELEMNAME_WITHPARAM_STRING] = Constants::ELEMNAME_WITHPARAM;
	theElementKeys[Constants::ELEMNAME_CONSTRUCT_STRING] = Constants::ELEMNAME_CONSTRUCT;
	theElementKeys[Constants::ELEMNAME_CONTENTS_STRING] = Constants::ELEMNAME_CONTENTS;
	theElementKeys[Constants::ELEMNAME_COPY_STRING] = Constants::ELEMNAME_COPY;
	theElementKeys[Constants::ELEMNAME_COPY_OF_STRING] = Constants::ELEMNAME_COPY_OF;

	theElementKeys[Constants::ELEMNAME_ATTRIBUTESET_STRING] = Constants::ELEMNAME_DEFINEATTRIBUTESET;

	theElementKeys[Constants::ELEMNAME_USE_STRING] = Constants::ELEMNAME_USE;

	theElementKeys[Constants::ELEMNAME_VARIABLE_STRING] = Constants::ELEMNAME_VARIABLE;
	theElementKeys[Constants::ELEMNAME_PARAMVARIABLE_STRING] = Constants::ELEMNAME_PARAMVARIABLE;

	theElementKeys[Constants::ELEMNAME_DISPLAYIF_STRING] = Constants::ELEMNAME_DISPLAYIF;
	theElementKeys[Constants::ELEMNAME_EMPTY_STRING] = Constants::ELEMNAME_EMPTY;
	theElementKeys[Constants::ELEMNAME_EVAL_STRING] = Constants::ELEMNAME_EVAL;
	theElementKeys[Constants::ELEMNAME_CALLTEMPLATE_STRING] = Constants::ELEMNAME_CALLTEMPLATE;
	theElementKeys[Constants::ELEMNAME_TEMPLATE_STRING] = Constants::ELEMNAME_TEMPLATE;
	theElementKeys[Constants::ELEMNAME_STYLESHEET_STRING] = Constants::ELEMNAME_STYLESHEET;
	theElementKeys[Constants::ELEMNAME_TRANSFORM_STRING] = Constants::ELEMNAME_STYLESHEET;
	theElementKeys[Constants::ELEMNAME_IMPORT_STRING] = Constants::ELEMNAME_IMPORT;
	theElementKeys[Constants::ELEMNAME_INCLUDE_STRING] = Constants::ELEMNAME_INCLUDE;
	theElementKeys[Constants::ELEMNAME_FOREACH_STRING] = Constants::ELEMNAME_FOREACH;
	theElementKeys[Constants::ELEMNAME_VALUEOF_STRING] = Constants::ELEMNAME_VALUEOF;
	theElementKeys[Constants::ELEMNAME_KEY_STRING] = Constants::ELEMNAME_KEY;
	theElementKeys[Constants::ELEMNAME_STRIPSPACE_STRING] = Constants::ELEMNAME_STRIPSPACE;
	theElementKeys[Constants::ELEMNAME_PRESERVESPACE_STRING] = Constants::ELEMNAME_PRESERVESPACE;
	theElementKeys[Constants::ELEMNAME_NUMBER_STRING] = Constants::ELEMNAME_NUMBER;
	theElementKeys[Constants::ELEMNAME_IF_STRING] = Constants::ELEMNAME_IF;
	theElementKeys[Constants::ELEMNAME_CHOOSE_STRING] = Constants::ELEMNAME_CHOOSE;
	theElementKeys[Constants::ELEMNAME_WHEN_STRING] = Constants::ELEMNAME_WHEN;
	theElementKeys[Constants::ELEMNAME_OTHERWISE_STRING] = Constants::ELEMNAME_OTHERWISE;
	theElementKeys[Constants::ELEMNAME_TEXT_STRING] = Constants::ELEMNAME_TEXT;
	theElementKeys[Constants::ELEMNAME_ELEMENT_STRING] = Constants::ELEMNAME_ELEMENT;
	theElementKeys[Constants::ELEMNAME_ATTRIBUTE_STRING] = Constants::ELEMNAME_ATTRIBUTE;
	theElementKeys[Constants::ELEMNAME_SORT_STRING] = Constants::ELEMNAME_SORT;
	theElementKeys[Constants::ELEMNAME_PI_STRING] = Constants::ELEMNAME_PI;
	theElementKeys[Constants::ELEMNAME_COMMENT_STRING] = Constants::ELEMNAME_COMMENT;
   
	theElementKeys[Constants::ELEMNAME_COUNTER_STRING] = Constants::ELEMNAME_COUNTER;
	theElementKeys[Constants::ELEMNAME_COUNTERS_STRING] = Constants::ELEMNAME_COUNTERS;
	theElementKeys[Constants::ELEMNAME_COUNTERINCREMENT_STRING] = Constants::ELEMNAME_COUNTERINCREMENT;
	theElementKeys[Constants::ELEMNAME_COUNTERRESET_STRING] = Constants::ELEMNAME_COUNTERRESET;
	theElementKeys[Constants::ELEMNAME_COUNTERSCOPE_STRING] = Constants::ELEMNAME_COUNTERSCOPE;
	
	theElementKeys[Constants::ELEMNAME_APPLY_IMPORTS_STRING] = Constants::ELEMNAME_APPLY_IMPORTS;
	
	theElementKeys[Constants::ELEMNAME_EXTENSION_STRING] = Constants::ELEMNAME_EXTENSION;
	theElementKeys[Constants::ELEMNAME_MESSAGE_STRING] = Constants::ELEMNAME_MESSAGE;
	theElementKeys[Constants::ELEMNAME_LOCALE_STRING] = Constants::ELEMNAME_LOCALE;
	theElementKeys[Constants::ELEMNAME_FALLBACK_STRING] = Constants::ELEMNAME_FALLBACK;
	theElementKeys[Constants::ELEMNAME_OUTPUT_STRING] = Constants::ELEMNAME_OUTPUT;

	theElementKeys[Constants::ELEMNAME_DECIMALFORMAT_STRING] = Constants::ELEMNAME_DECIMALFORMAT;
	theElementKeys[Constants::ELEMNAME_NSALIAS_STRING] = Constants::ELEMNAME_NSALIAS;
}



void
XSLTEngineImpl::initializeXSLT4JElementKeys(ElementKeysMapType&		theElementKeys)
{
	theElementKeys[Constants::ELEMNAME_COMPONENT_STRING] = Constants::ELEMNAME_COMPONENT;
	theElementKeys[Constants::ELEMNAME_SCRIPT_STRING] = Constants::ELEMNAME_SCRIPT;
}



static XalanDOMString							s_XSLNameSpaceURL;

static XalanDOMString							s_XSLT4JNameSpaceURL;

static XalanDOMString							s_uniqueNamespacePrefix;

static XSLTEngineImpl::AttributeKeysMapType		s_attributeKeys;

static XSLTEngineImpl::ElementKeysMapType		s_elementKeys;

static XSLTEngineImpl::ElementKeysMapType		s_XSLT4JElementKeys;



const double			XSLTEngineImpl::s_XSLTVerSupported(1.0);

const XalanDOMString&	XSLTEngineImpl::s_XSLNameSpaceURL = ::s_XSLNameSpaceURL;

const XalanDOMString&	XSLTEngineImpl::s_XSLT4JNameSpaceURL = ::s_XSLT4JNameSpaceURL;

const XalanDOMString&	XSLTEngineImpl::s_uniqueNamespacePrefix = ::s_uniqueNamespacePrefix;


/**
 * Control if the xsl:variable is resolved early or 
 * late. Resolving the xsl:variable
 * early is a drag because it means that the fragment 
 * must be created into a DocumentFragment, and then 
 * cloned each time to the result tree.  If we can resolve 
 * late, that means we can evaluate directly into the 
 * result tree.  Right now this must be kept on 'early' 
 * because you would need to set the call stack back to 
 * the point of xsl:invoke... which I can't quite work out 
 * at the moment.  I don't think this is worth fixing 
 * until NodeList variables are implemented.
 */
const bool										XSLTEngineImpl::s_resolveContentsEarly = true;

const XSLTEngineImpl::AttributeKeysMapType&		XSLTEngineImpl::s_attributeKeys = ::s_attributeKeys;

const XSLTEngineImpl::ElementKeysMapType&		XSLTEngineImpl::s_elementKeys = ::s_elementKeys;

const XSLTEngineImpl::ElementKeysMapType&		XSLTEngineImpl::s_XSLT4JElementKeys = ::s_XSLT4JElementKeys;



void
XSLTEngineImpl::initialize()
{
	::s_XSLNameSpaceURL = XALAN_STATIC_UCODE_STRING("http://www.w3.org/1999/XSL/Transform");

	::s_XSLT4JNameSpaceURL = XALAN_STATIC_UCODE_STRING("http://xml.apache.org/xslt");

	::s_uniqueNamespacePrefix = XALAN_STATIC_UCODE_STRING("ns");

	installFunctions();

	initializeAttributeKeysTable(::s_attributeKeys);

	initializeElementKeysTable(::s_elementKeys);

	initializeXSLT4JElementKeys(::s_XSLT4JElementKeys);
}



void
XSLTEngineImpl::terminate()
{
	ElementKeysMapType().swap(::s_XSLT4JElementKeys);

	ElementKeysMapType().swap(::s_elementKeys);

	AttributeKeysMapType().swap(::s_attributeKeys);

	uninstallFunctions();

	clear(::s_uniqueNamespacePrefix);

	clear(::s_XSLT4JNameSpaceURL);

	clear(::s_XSLNameSpaceURL);
}
