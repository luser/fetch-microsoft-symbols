/*
 * Copyright 2014 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "cabextract.h"

const char kMSSymbolServer[] = "http://msdl.microsoft.com/download/symbols/";
const char kUserAgent[] = "Microsoft-Symbol-Server/6.3.0.0";
const char kCABHeader[] = "MSCF";
const char kCompressHeader[] = "SZDD";

using std::string;
using std::vector;

size_t save_bytes_to_vector(char* ptr, size_t size, size_t nmemb,
                            void* userdata) {
  vector<uint8_t>* vec = reinterpret_cast<vector<uint8_t>*>(userdata);
  vec->insert(vec->end(), ptr, ptr + (size * nmemb));
  return size * nmemb;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: fetch_microsoft_symbols <debug file> <debug id>\n");
    return 1;
  }
  const char* debug_file = argv[1];
  const char* debug_id = argv[2];

  CURL* curl = curl_easy_init();

  string url = kMSSymbolServer;
  url += debug_file;
  url += "/";
  url += debug_id;
  url += "/";
  url += debug_file;
  // Try the compressed version, this always works AFAICT.
  url[url.length() - 1] = '_';

  vector<uint8_t> compressed_bytes;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
  curl_easy_setopt(curl, CURLOPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_STDERR, stderr);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_bytes_to_vector);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &compressed_bytes);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);

  long retcode = -1;
  if (curl_easy_perform(curl) != 0 ||
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode) != 0 ||
      retcode != 200) {
    fprintf(stderr, "Failed to fetch %s\n", url.c_str());
  }
  curl_easy_cleanup(curl);

  // Sanity check that this is a cabinet file
  if (compressed_bytes.size() < 4 ||
      memcmp(&compressed_bytes[0], kCABHeader, 4) != 0) {
    fprintf(stderr, "Not a cabinet file!\n");
    return 1;
  }

  vector<uint8_t> pdb_bytes;
  if (!extract_cab(compressed_bytes, pdb_bytes)) {
    fprintf(stderr, "Failed to decompress PDB\n");
    return 1;
  }

  //TODO: dump symbols from PDB
  return 0;
}

