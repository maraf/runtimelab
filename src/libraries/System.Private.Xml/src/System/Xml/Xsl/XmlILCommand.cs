// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// <spec>http://webdata/xml/specs/querylowlevel.xml</spec>
//------------------------------------------------------------------------------

using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Xml.XPath;
using System.Xml.Xsl.Runtime;
using System.Runtime.Versioning;

namespace System.Xml.Xsl
{
    /// <summary>
    /// This is the executable command generated by the XmlILGenerator.
    /// </summary>
    internal sealed class XmlILCommand
    {
        private readonly ExecuteDelegate _delExec;
        private readonly XmlQueryStaticData _staticData;

        /// <summary>
        /// Constructor.
        /// </summary>
        public XmlILCommand(ExecuteDelegate delExec, XmlQueryStaticData staticData)
        {
            Debug.Assert(delExec != null && staticData != null);
            _delExec = delExec;
            _staticData = staticData;
        }

        /// <summary>
        /// Return query static data required by the runtime.
        /// </summary>
        public XmlQueryStaticData StaticData
        {
            get { return _staticData; }
        }


        /// <summary>
        /// Execute the dynamic assembly generated by the XmlILGenerator.
        /// </summary>
        public void Execute(object defaultDocument, XmlResolver? dataSources, XsltArgumentList? argumentList, XmlWriter writer)
        {
            try
            {
                if (writer is XmlAsyncCheckWriter)
                {
                    writer = ((XmlAsyncCheckWriter)writer).CoreWriter;
                }

                // Try to extract a RawWriter
                XmlWellFormedWriter? wellFormedWriter = writer as XmlWellFormedWriter;

                if (wellFormedWriter != null &&
                    wellFormedWriter.RawWriter != null &&
                    wellFormedWriter.WriteState == WriteState.Start &&
                    wellFormedWriter.Settings.ConformanceLevel != ConformanceLevel.Document)
                {
                    // Extracted RawWriter from WellFormedWriter
                    Execute(defaultDocument, dataSources, argumentList, new XmlMergeSequenceWriter(wellFormedWriter.RawWriter));
                }
                else
                {
                    // Wrap Writer in RawWriter
                    Execute(defaultDocument, dataSources, argumentList, new XmlMergeSequenceWriter(new XmlRawWriterWrapper(writer)));
                }
            }
            finally
            {
                writer.Flush();
            }
        }

        /// <summary>
        /// Execute the dynamic assembly generated by the XmlILGenerator.
        /// </summary>
        private void Execute(object defaultDocument, XmlResolver? dataSources, XsltArgumentList? argumentList, XmlSequenceWriter results)
        {
            Debug.Assert(results != null);

            // Ensure that dataSources is always non-null
            dataSources ??= XmlResolver.ThrowingResolver;

            _delExec(new XmlQueryRuntime(_staticData, defaultDocument, dataSources, argumentList, results));
        }
    }
}
