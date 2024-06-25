import React from 'react';
import styled from 'styled-components';
import { Menu } from 'semantic-ui-react';
import {Home} from '../App';
import {Config} from '../App';
import {HomeIcon, ConfigurationIcon} from 'practical-react-components-icons'

import {PracticalProvider} from 'practical-react-components-core'
import { Text } from 'react-native'

const StyledSideNav = styled.div`   
    .menu {
        position: fixed;
        top: 50px;
        height: 100%;
        background-color: #e6e6e6;
        border: 0px;
    }
`;

class SidebarPage extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            activeItem: 'Home',
        }
    }

    // ** EDIT ** itens lists
    render() {
        const { activeItem } = this.state

        return(
          <PracticalProvider>
            <StyledSideNav>
            <Menu vertical>
            
            <Menu.Item
              name='Home'
              onClick={Home}
              //as={Link}
              //to="/"
                            
            >
              <HomeIcon> </HomeIcon>
              <Text>Home</Text>              
            </Menu.Item>
            
            <Menu.Item
              name='Configuration'
              onClick={Config}
              //as={Link}
              //to="/Config"
            >
              <ConfigurationIcon></ConfigurationIcon>
              <Text>Configuration</Text>
            </Menu.Item>


            {/*<Menu.Item
              name='Status'
              onClick={this.handleItemClick}
              as={Link}
              to="/NoMatch"
            />*/}
          </Menu>
          </StyledSideNav>
          </PracticalProvider>
        );
    }
}


export default SidebarPage; 



