import json, unittest
from collections import Counter
from pathlib import Path
BASE=Path(__file__).resolve().parents[1]/'data'
def load(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
class TestCatalogo(unittest.TestCase):
    def test_jsons(self):
        for name in ['municipios_coredes_preliminar.json','coredes_regioes_funcionais.json','biomas_rs.json','catalogo_fontes.json']:
            self.assertIsInstance(load(name),dict)
    def test_municipios_sem_duplicatas(self):
        m=load('municipios_coredes_preliminar.json')['municipios'];self.assertEqual(len(m),497)
        self.assertEqual(len(set(x['nome'] for x in m)),497)
    def test_rf(self):
        ms=load('municipios_coredes_preliminar.json')['municipios'];cs=load('coredes_regioes_funcionais.json')
        self.assertEqual(len(cs['coredes']),28);self.assertEqual(len(cs['regioes_funcionais']),9)
        self.assertEqual([Counter(m['regiao_funcional_id'] for m in ms)[f'RF{i}'] for i in range(1,10)], [70,59,49,21,22,20,77,49,130])
        mapping={c['id']:c['regiao_funcional_id'] for c in cs['coredes']}
        self.assertTrue(all(mapping[m['corede_id']]==m['regiao_funcional_id'] for m in ms))
    def test_incompleto_explicitamente(self):
        doc=load('municipios_coredes_preliminar.json')
        self.assertIn('PRELIMINAR',doc['estado'])
        self.assertEqual(sum(m['bioma_predominante_id'] is None for m in doc['municipios']),495)
        self.assertTrue(all(m['biomas_presentes_ids'] is None or isinstance(m['biomas_presentes_ids'],list) for m in doc['municipios']))
    def test_exemplos_ibge(self):
        ms={m['nome']:m for m in load('municipios_coredes_preliminar.json')['municipios']}
        self.assertEqual((ms['Caibaté']['codigo_ibge'],ms['Caibaté']['bioma_predominante_id']),('4303301','pampa'))
        self.assertEqual((ms['Campina das Missões']['codigo_ibge'],ms['Campina das Missões']['bioma_predominante_id']),('4303707','mata-atlantica'))
if __name__=='__main__': unittest.main()
