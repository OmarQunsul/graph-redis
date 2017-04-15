require 'redis'

redis = Redis.new

describe 'Removing nodes' do
  it 'should correctly remove nodes from a undirected graph' do
    redis.flushdb
    #redis.gsetdirected 'graph1'
    redis.gnode 'graph1', 'a', 'b', 'c', 'd'
    redis.gedge 'graph1', 'a', 'b', 1
    b_neighbours = redis.gneighbours 'graph1', 'b'
    expect(b_neighbours).to eq(['a'])
    expect((redis.gedges 'graph1').length).to eq(1 * 3)

    response = redis.gnoderem 'graph1', 'a'

    expect(response).to eq(1)
    b_neighbours = redis.gneighbours 'graph1', 'b'
    expect(b_neighbours).to eq([])

    nodes = redis.gnodes 'graph1', '*'
    expect(nodes).to eq(%w[b c d])

    response = redis.gnoderem 'graph1', 'a'
    expect(response).to eq(0)

    expect((redis.gedges 'graph1').length).to eq(0 * 3)
  end

  it 'should correctly remove edges from directed graphs' do
  end
end
