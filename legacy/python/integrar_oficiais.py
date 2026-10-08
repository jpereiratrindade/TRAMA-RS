#!/usr/bin/env python3
"""Importacao offline de tabelas IBGE/Atlas. So publica catalogo integrado se completo.

Uso:
 python3 tools/integrar_oficiais.py --atlas-xlsx raw/coredes.xlsx --ibge-csv raw/biomas_2024.csv [--presenca-xls raw/biomas_2019.xls]

O importador usa apenas a biblioteca padrao para xlsx/csv. Opcional: pip install xlrd>=2.0 para .xls.
"""
import argparse, csv, io, json, re, unicodedata, zipfile
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'data'; OUTPUT=DATA/'gerados'
NS={'m':'http://schemas.openxmlformats.org/spreadsheetml/2006/main','r':'http://schemas.openxmlformats.org/officeDocument/2006/relationships'}

def norm(t):
    t=unicodedata.normalize('NFKD',str(t or ''))
    return re.sub(r'\s+',' ', ''.join(c for c in t if not unicodedata.combining(c)).lower()).strip()

def rows_xlsx(path):
    """Lê todas as planilhas XLSX em XML sem alterar o arquivo original."""
    with zipfile.ZipFile(path) as z:
        shared=[]
        if 'xl/sharedStrings.xml' in z.namelist():
            root=ET.fromstring(z.read('xl/sharedStrings.xml'))
            shared=[''.join(x.text or '' for x in si.findall('.//m:t',NS)) for si in root.findall('m:si',NS)]
        for sheet in sorted(n for n in z.namelist() if re.fullmatch(r'xl/worksheets/sheet\d+\.xml',n)):
            root=ET.fromstring(z.read(sheet))
            for row in root.findall('.//m:sheetData/m:row',NS):
                cells=[]
                for c in row.findall('m:c',NS):
                    val=c.find('m:v',NS)
                    if c.attrib.get('t')=='inlineStr':
                        value=''.join(x.text or '' for x in c.findall('.//m:t',NS))
                    elif val is not None:
                        value=shared[int(val.text)] if c.attrib.get('t')=='s' else (val.text or '')
                    else: value=''
                    cells.append(str(value).strip())
                yield sheet,cells

def parse_atlas(path, base):
    """Varre todas as células, detecta identificadores e nomes, com RF/COREDE por linha ou cabeçalho de grupo.

    Se layout nao permitir identificar 497 linhas, interrompe em vez de adivinhar.
    """
    names={norm(m['nome']):m for m in base}
    corede_info=json.loads((DATA/'coredes_regioes_funcionais.json').read_text())['coredes']
    coredes={norm(c['nome']):c for c in corede_info}
    aliases={'paranhana/encosta da serra':'paranhana-encosta da serra','metropolitano do delta do jacui':'metropolitano delta do jacui',
             'centro sul':'centro-sul','jacui-centro':'jacui centro','litoral norte':'litoral'}
    assigned={}; codigos={}; context_rf=None; context_corede=None
    for sheet,row in rows_xlsx(path):
        if not any(row):continue
        rf=next((f'RF{n}' for v in row for n in re.findall(r'\bRF\s*([1-9])\b',norm(v).upper())),None)
        found_corede=None; found_municipio=None; code=None
        for v in row:
            nv=norm(v)
            nv=aliases.get(nv,nv)
            if nv in coredes:found_corede=coredes[nv]
            if nv in names:found_municipio=names[nv]['nome']
            m=re.fullmatch(r'([0-9]{7})(?:\.0)?',v.strip())
            if m and m.group(1).startswith('43'):code=m.group(1)
        if rf: context_rf=rf
        if found_corede:context_corede=found_corede
        if found_municipio:
            if found_municipio in assigned:raise RuntimeError(f'Municipio repetido no Atlas: {found_municipio}')
            if context_corede is None:raise RuntimeError(f'COREDE nao identificado no Atlas para {found_municipio}')
            target_rf=context_corede['regiao_funcional_id']
            if context_rf and target_rf != context_rf:
                raise RuntimeError(f'RF incongruente para {found_municipio}: {context_rf} vs {target_rf}')
            assigned[found_municipio]=context_corede['id']
            if code:codigos[found_municipio]=code
    if len(assigned)!=497:
        missing=sorted(set(m['nome'] for m in base)-set(assigned))
        raise RuntimeError(f'Atlas: extracao incompleta ({len(assigned)}/497). Conferir layout do XLSX. Exemplos faltantes: {missing[:20]}')
    return assigned,codigos

def parse_csv_ibge(path):
    data=None
    for encoding in ['utf-8-sig','cp1252','latin-1']:
        try:data=path.read_text(encoding=encoding);break
        except UnicodeError:pass
    if data is None:raise ValueError('Codificacao nao reconhecida')
    # Remove linhas vazias e títulos que eventualmente precedam o cabecalho.
    lines=[line for line in data.splitlines() if line.strip()]
    delim=max([';',',','\t'],key=lambda x:sum(line.count(x) for line in lines[:30]))
    rows=list(csv.reader(lines,delimiter=delim))
    match=[]
    for row in rows:
        joined=norm(' '.join(row))
        if any(x in joined for x in ['geocodigo','codigo do municipio','codmun','codigo ibge']) and 'bioma' in joined:
            match=row;break
    if not match:raise RuntimeError('IBGE 2024: cabecalho nao identificado; verificar nomes das colunas')
    start=rows.index(match)+1
    headers=[norm(v) for v in match]
    def ix(*patterns):
        for i,h in enumerate(headers):
            if any(p in h for p in patterns):return i
        return None
    ci=ix('geocodigo','codigo ibge','codmun','codigo do municipio')
    mi=ix('nome do municipio','municipio','nome_mun')
    bi=ix('bioma predominante','bioma_predominante','bioma')
    ufi=ix('sigla da uf','uf')
    if ci is None or bi is None:raise RuntimeError(f'Colunas esperadas ausentes: {headers}')
    resultado={}
    for row in rows[start:]:
        if len(row)<=max(ci,bi):continue
        code=re.sub('[^0-9]','',row[ci])
        if len(code)!=7 or not code.startswith('43') or code in ('4300001','4300002'):continue
        if ufi is not None and len(row)>ufi and norm(row[ufi]) not in ['rs','rio grande do sul']:continue
        biome=norm(row[bi]);
        if 'pampa' in biome:bio='pampa'
        elif 'mata atlantica' in biome:bio='mata-atlantica'
        else:raise ValueError(f'Bioma nao reconhecido para {code}: {row[bi]}')
        resultado[code]={'bioma_predominante_id':bio,'nome_ibge':row[mi] if mi is not None and len(row)>mi else None}
    if len(resultado)!=497:raise RuntimeError(f'IBGE 2024: {len(resultado)}/497 municipios (excluidas lagoas). Conferir CSV/colunas')
    return resultado

def parse_xls_presenca(path, codes, names):
    try:import xlrd
    except ImportError as e:raise RuntimeError('Para importar XLS 2019 instale: python3 -m pip install xlrd>=2.0') from e
    book=xlrd.open_workbook(str(path)); by_code=defaultdict(set)
    by_name={norm(m['nome']):m['codigo_ibge'] for m in names}
    for sheet in book.sheets():
        for i in range(sheet.nrows):
            vals=[str(sheet.cell_value(i,j)).strip() for j in range(sheet.ncols)]
            joined=' '.join(vals)
            found=set()
            if re.search(r'Pampa',joined,re.I):found.add('pampa')
            if re.search(r'Mata Atl[aâ]ntica',joined,re.I):found.add('mata-atlantica')
            if not found:continue
            code=next((m.group(1) for v in vals if (m:=re.fullmatch(r'(43[0-9]{5})(?:\.0)?',v))),None)
            if not code:
                code=next((by_name[norm(v)] for v in vals if norm(v) in by_name),None)
            if code in codes:by_code[code].update(found)
    if len(by_code)!=497:
        raise RuntimeError(f'IBGE 2019: apenas {len(by_code)}/497 municipios extraidos; verificar tabela e formato antes de publicar')
    return by_code

def main():
    a=argparse.ArgumentParser()
    a.add_argument('--atlas-xlsx',type=Path,required=True)
    a.add_argument('--ibge-csv',type=Path,required=True)
    a.add_argument('--presenca-xls',type=Path)
    args=a.parse_args()
    base=json.loads((DATA/'municipios_coredes_preliminar.json').read_text())['municipios']
    assigned, codes_xlsx=parse_atlas(args.atlas_xlsx,base)
    ibge=parse_csv_ibge(args.ibge_csv)
    by_name_ibge={norm(v['nome_ibge']):code for code,v in ibge.items() if v['nome_ibge']}
    municipios=[]
    corede_info={c['id']:c for c in json.loads((DATA/'coredes_regioes_funcionais.json').read_text())['coredes']}
    for item in base:
        m=item.copy();name=m['nome']
        m['corede_id']=assigned[name]
        m['regiao_funcional_id']=corede_info[assigned[name]]['regiao_funcional_id']
        code=codes_xlsx.get(name) or by_name_ibge.get(norm(name))
        if code not in ibge:raise RuntimeError(f'Geocodigo IBGE nao identificado: {name} / {code}')
        m['codigo_ibge']=code
        m['bioma_predominante_id']=ibge[code]['bioma_predominante_id']
        m['biomas_presentes_ids']=None
        m['classificacao_bioma_status']='predominante_ibge_2024_verificado'
        municipios.append(m)
    if len(set(m['codigo_ibge'] for m in municipios))!=497:raise RuntimeError('Geocodigos duplicados ou ausentes')
    if args.presenca_xls:
        presentes=parse_xls_presenca(args.presenca_xls,set(ibge),municipios)
        for m in municipios:
            m['biomas_presentes_ids']=sorted(presentes[m['codigo_ibge']]);m['classificacao_bioma_status']='predominante_2024_presenca_2019'
    OUTPUT.mkdir(exist_ok=True)
    mun_doc={'schema_version':'1.0.0','status':'INTEGRADO_COM_FONTES_PRIMARIAS','fontes':{
        'municipios_coredes':str(args.atlas_xlsx),'bioma_predominante_2024':str(args.ibge_csv),'biomas_presentes_2019':str(args.presenca_xls) if args.presenca_xls else None},
        'total':497,'municipios':sorted(municipios,key=lambda m:m['codigo_ibge'])}
    (OUTPUT/'municipios_rs.json').write_text(json.dumps(mun_doc,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    for campo,saida in [('bioma_predominante_id','municipios_por_bioma_predominante_2024.json'),('biomas_presentes_ids','municipios_por_bioma_presenca_2019.json')]:
        if campo=='biomas_presentes_ids' and not args.presenca_xls:continue
        grupos={id:[] for id in ('pampa','mata-atlantica')}
        for m in municipios:
            values=m[campo] if isinstance(m[campo],list) else [m[campo]]
            for v in values:
                if v:grupos[v].append(m['codigo_ibge'])
        (OUTPUT/saida).write_text(json.dumps({'tipo':campo,'municipios_por_bioma':grupos,'quantidades':{k:len(v) for k,v in grupos.items()}},ensure_ascii=False,indent=2)+'\n')
    print('OK: 497 codigos IBGE distintos + COREDE/RF oficiais + bioma predominante oficial; presenca 2019:',bool(args.presenca_xls))

if __name__=='__main__':main()
