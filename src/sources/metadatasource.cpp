#include "metadatasource.h"

MetadataSource::MetadataSource(HttpClient* client, QObject* parent)
    : QObject(parent)
    , m_httpClient(client)
{
}
