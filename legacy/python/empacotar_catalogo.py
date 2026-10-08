#!/usr/bin/env python3
"""Consolida e gera manifest deterministico de integridade dos JSON individuais."""
from pathlib import Path
import hashlib, json
p=Path(__file__).resolve().parents[1]/'data'
parts={x:x2 for x,x2 in [('coredes_regioes_funcionais.json','regionalizacao'),('municipios_coredes_preliminar.json','municipios'),('biomas_rs.json','biomas'),('catalogo_fontes.json','proveniencia')]}
content={alias:json.loads((p/name).read_text(encoding='utf-8')) for name,alias in parts.items()}
bundle={'schema_version':'1.0.0','status':'PRELIMINAR_NAO_HOMOLOGADO','total_municipios':497,**content}
(p/'catalogo_territorio_rs_preliminar.json').write_text(json.dumps(bundle,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=list(parts)+['catalogo_territorio_rs_preliminar.json']
manifest={'schema_version':'1.0.0','arquivos':[{'nome':n,'sha256':hashlib.sha256((p/n).read_bytes()).hexdigest(),'bytes':(p/n).stat().st_size} for n in files]}
(p/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
print('OK: catalogo consolidado e manifest de integridade criados')
