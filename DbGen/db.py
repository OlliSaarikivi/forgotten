table('body',
      ('id', 'uint32_t'),
      ('ptr', 'b2Body*'),
      primary_key='id')

table('target',
      ('id', 'uint32_t'),
      ('target', 'uint32_t'))

view('bodyAndTarget',
     select(('id', 'body.id'), #TODO
        merge_join_using('id', 'body', 'target')))

index('bodyAndTarget', 'target')

query('targetting',
      merge_join(('bodyAndTarget', 'target')
                 (alias('body', 'targetBody'), 'id')))
