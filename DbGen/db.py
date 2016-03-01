table('embodied',
      ('id', 'uint32_t'),
      ('body', 'b2Body*'),
      key='id')

table('targeter',
      ('id', 'uint32_t'),
      ('target', 'uint32_t'))

view('bodyAndTarget',
     ('targeterBody', 'embodied.body'),
     ('target', 'targeter.target'),
     merge_join_using('id', 'embodied', 'targeter'))

index('bodyAndTarget', 'target')

query('targetting',
      merge_join(('bodyAndTarget', 'target')
                 (alias('body', 'targetBody'), 'id')))
