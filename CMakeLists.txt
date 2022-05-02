install(
  FILES AddCatalogSourcesDialog.py CatalogSourceExtractor.py DS9Connector.py Element.py
  ElementEditor.py FitsFile.py HelpWindow.py ModelEditor.py Parameter.py
  ParameterEditor.py ParameterSet.py Source.py SourceEditor.py SourceLibrary.py
  SourceLibraryDocument.py SourceLibraryDocumentEditor.py SourceLibraryEditor.py
  SpatialModel.py SpatialModelEditor.py Spectrum.py SpectrumEditor.py
  
  DESTINATION ${FERMI_INSTALL_PYTHON}
)

install(FILES source_library.xsd source_library_obssim.xsd DESTINATION ${FERMI_INSTALL_DATADIR}/modelEditor)
install(FILES source_library.xsd source_library_obssim.xsd DESTINATION ${FERMI_INSTALL_REFDATADIR}/modelEditor)
