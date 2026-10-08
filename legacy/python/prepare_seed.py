#!/usr/bin/env python3
"""Gera arquivos preliminares auditáveis, sem inventar geocódigos/biomas."""
from pathlib import Path
import json, re, unicodedata, collections
ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'data'
ATLAS='https://atlassocioeconomico.rs.gov.br/regioes-funcionais-de-planejamento'
XLSX='https://atlassocioeconomico.rs.gov.br/upload/arquivos/202010/09172616-tabela-dos-municipios-por-corede-e-regiao-funcional-de-planejamento.xlsx'
IBGE='https://geoftp.ibge.gov.br/informacoes_ambientais/estudos_ambientais/biomas/documentos/'
WIKI='https://newikis.com/pt/Conselhos_Regionais_de_Desenvolvimento'
def slug(s):
    s=unicodedata.normalize('NFKD',s)
    s=''.join(c for c in s if not unicodedata.combining(c))
    return re.sub('-+', '-', re.sub('[^a-z0-9]+','-',s.lower())).strip('-')
def write(file,data):
    (DATA/file).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
lines=(ROOT/'tools/municipios_seed.txt').read_text(encoding='utf-8').splitlines()
coredes=[]; municipios=[]
for line in lines:
    rf,corede,names=line.split('|')
    corede_id=slug(corede)
    coredes.append({'id':corede_id,'nome':corede,'regiao_funcional_id':rf,'municipios_quantidade_preliminar':len(names.split(';'))})
    for name in names.split(';'):
        m={'nome':name,'codigo_ibge':None,'uf':'RS','corede_id':corede_id,'regiao_funcional_id':rf,
           'bioma_predominante_id':None,'biomas_presentes_ids':None,'classificacao_bioma_status':'nao_integrado'}
        municipios.append(m)
assert len(coredes)==28
assert len(municipios)==497 and len(set(m['nome'] for m in municipios))==497
# Exemplos pontuais conferidos na publicação IBGE 2024, apêndice interbiomas.
known={
 'Caibaté':('4303301','pampa'),
 'Campina das Missões':('4303707','mata-atlantica')
}
for m in municipios:
    if m['nome'] in known:
        m['codigo_ibge'],m['bioma_predominante_id']=known[m['nome']]
        m['biomas_presentes_ids']=['mata-atlantica','pampa']
        m['classificacao_bioma_status']='exemplo_ibge_2024_interbiomas'
metadata={'schema_version':'1.0.0','dataset_version':'0.1.0','uf':'RS','estado':'PRELIMINAR_NAO_HOMOLOGADO',
 'observacao':'Atribuicoes municipais baseadas em compilacao secundaria para prototipacao. Confrontar com planilha oficial Atlas; dois exemplos de bioma IBGE, demais pendentes.',
 'fontes':[{'tipo':'regionalizacao_oficial','url':ATLAS,'arquivo':XLSX,'ano_referencia':2020},
           {'tipo':'seed_municipios_secundaria','url':WIKI,'licenca':'CC BY-SA 4.0','nota':'Revisar com fonte primaria antes de publicar como definitivo'},
           {'tipo':'bioma_predominante_ibge','url':IBGE+'Bioma_Predominante_por_Municipio_2024.csv','ano_referencia':2024},
           {'tipo':'biomas_presentes_ibge','url':IBGE+'Lista_Municipio_Bioma_250mil.xls','ano_referencia':2019}]}
write('coredes_regioes_funcionais.json',{**metadata,'regioes_funcionais':[{'id':f'RF{i}','numero':i,'coredes_ids':[c['id'] for c in coredes if c['regiao_funcional_id']==f'RF{i}']} for i in range(1,10)],'coredes':coredes})
write('municipios_coredes_preliminar.json',{**metadata,'total':len(municipios),'municipios':sorted(municipios,key=lambda x:slug(x['nome']))})
write('biomas_rs.json',{'schema_version':'1.0.0','uf':'RS','tipo':'biomas_terrestres_ibge','biomas':[{'id':'pampa','nome':'Pampa'},{'id':'mata-atlantica','nome':'Mata Atlântica'}],
 'criterios':{'predominante':'IBGE 2024: maior area do municipio','presenca':'IBGE 2019: todos os biomas que tocam o municipio; ano metodologicamente distinto','proporcao':'Nao disponivel nesta versao; exige intersecao geoespacial'},
 'fontes':[IBGE+'LEIA_ME_Sobre_a_relacao_entre_municipios_e_biomas.pdf',IBGE+'Bioma_Predominante_por_Municipio_2024.csv',IBGE+'Lista_Municipio_Bioma_250mil.xls']})
write('catalogo_fontes.json',{**metadata,'normalizacao':{'municipio_id':'codigo_ibge (7 digitos, texto)','corede_id':'slug estavel interno; nao confundir com codigo oficial','regiao_funcional_id':'RF1 a RF9','bioma_id':'pampa | mata-atlantica'},'invariantes':{'municipios':497,'coredes':28,'regioes_funcionais':9,'biomas':2},
 'politica':'Fontes primarias prevalecem sobre compilacao secundaria; nao inferir bioma por COREDE e nao converter null em ausencia.'})
print('OK: 497 municipios (preliminares), 28 COREDES, 9 RFs, 2 biomas; 2 amostras de bioma identificadas')
